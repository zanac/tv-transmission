#include "ServerSettingsDialog.h"
#include "Strings.h"

#define Uses_TDialog
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TSItem
#define Uses_TValidator
#define Uses_TRangeValidator
#define Uses_TEvent
#define Uses_TScreen
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

// Local to this dialog: scoped to its own handleEvent, same reasoning
// as similar local command constants elsewhere in this project (e.g.
// TrackerPeerWindow's own cmRefreshTrackers).
constexpr ushort cmTestPort = 230;
constexpr ushort cmUpdateBlocklist = 231;

// TStaticText with a settable body — the base class only ever takes
// its text at construction (see tstatict.cpp: text(newStr(aText))),
// with no setText() of its own, since a plain label normally never
// needs to change after being built. This one does: "Test port"/
// "Update blocklist" show their result INLINE once clicked, not via a
// popup (a popup for a result the user can just look at again a moment
// later, right there in the same dialog, would be one more thing to
// dismiss for no benefit). Same newStr()/delete[] convention the base
// class itself already uses for `text`, and the same "replace the
// pointer, then ask for a redraw" approach this project already uses
// for a TView's own title (see TorrentListWindow::updateTitleForConnectionState()).
class TResultLabel : public TStaticText {
public:
    TResultLabel(const TRect& bounds, TStringView aText) : TStaticText(bounds, aText) {}
    void setText(const std::string& s) {
        delete[] (char*)text;
        text = newStr(s.c_str());
        drawView();
    }
};

// The only dialog on this project that needs to act on a live
// TransmissionClient WHILE still open, rather than only reading its
// fields back once closed (see serverSettingsDialogResult() below,
// which every other field on this dialog still goes through) — "Test
// port"/"Update blocklist" each make an ordinary blocking RPC call
// (client_.testPort()/updateBlocklist(), same call() and so same 15s
// timeout as every other action in this app) and show the result
// INLINE, without closing the dialog. flushScreen() forces the
// "Testing..."/"Updating..." text to actually reach the terminal
// BEFORE that blocking call starts — drawView() alone only updates
// tvision's own in-memory screen buffer, which wouldn't otherwise
// reach the real terminal until control returns from this same
// handleEvent() call, by which point the blocking call has already
// finished and there'd be nothing left to show it for.
class ServerSettingsDialogImpl : public TDialog {
public:
    ServerSettingsDialogImpl(const TRect& bounds, TStringView title, TransmissionClient& client)
        : TWindowInit(&TDialog::initFrame), TDialog(bounds, title), client_(client) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what != evCommand) return;
        switch (event.message.command) {
            case cmTestPort: {
                if (portLabel) {
                    portLabel->setText(tr(Str::ResultTesting));
                    TScreen::flushScreen();
                }
                bool portOpen = false;
                bool ok = client_.testPort(&portOpen);
                if (portLabel) {
                    portLabel->setText(!ok ? tr(Str::ResultPortTestFailed)
                                            : tr(portOpen ? Str::ResultPortOpen : Str::ResultPortClosed));
                }
                clearEvent(event);
                break;
            }
            case cmUpdateBlocklist: {
                if (blocklistLabel) {
                    blocklistLabel->setText(tr(Str::ResultUpdating));
                    TScreen::flushScreen();
                }
                int ruleCount = 0;
                bool ok = client_.updateBlocklist(&ruleCount);
                if (blocklistLabel) {
                    if (!ok) {
                        blocklistLabel->setText(tr(Str::ResultBlocklistFailed));
                    } else {
                        char buf[64];
                        std::snprintf(buf, sizeof(buf), tr(Str::ResultBlocklistUpdated), ruleCount);
                        blocklistLabel->setText(buf);
                    }
                }
                clearEvent(event);
                break;
            }
        }
    }

    // Set by createServerSettingsDialog() once built.
    TResultLabel* portLabel = nullptr;
    TResultLabel* blocklistLabel = nullptr;

private:
    TransmissionClient& client_;
};

// Shared by both the global and the alt-speed sections: an input line
// pre-filled with `value`, validated to a sane KB/s range.
TInputLine* addLimitField(TDialog* dlg, TRect r, int value) {
    auto* input = new TInputLine(r, 8);
    std::vector<char> buf(9, 0);
    std::snprintf(buf.data(), buf.size(), "%d", value);
    input->setData(buf.data());
    input->setValidator(new TRangeValidator(0, 1000000)); // KB/s, ~1GB/s cap
    dlg->insert(input);
    return input;
}

} // namespace

TDialog* createServerSettingsDialog(const SessionLimits& sessionLimits,
                                     ServerSettingsDialogFields& fields,
                                     TransmissionClient& client) {
    TRect r(0, 0, 60, 25);
    auto* dlg = new ServerSettingsDialogImpl(r, tr(Str::DialogTitleServerSettings), client);
    dlg->options |= ofCentered;

    // --- Global (session-wide) speed limits ---
    dlg->insert(new TStaticText(TRect(2, 2, 56, 3), tr(Str::LabelGlobalSpeedSection)));

    fields.globalLimitCheckboxes = new TCheckBoxes(TRect(2, 3, 26, 5),
        new TSItem(tr(Str::CheckGlobalLimitDownload),
        new TSItem(tr(Str::CheckGlobalLimitUpload), nullptr)));
    ushort globalChecked = (sessionLimits.downloadLimited ? 0x01 : 0) |
                           (sessionLimits.uploadLimited ? 0x02 : 0);
    fields.globalLimitCheckboxes->setData(&globalChecked);
    dlg->insert(fields.globalLimitCheckboxes);

    fields.globalDownloadLimit = addLimitField(dlg, TRect(28, 3, 38, 4), sessionLimits.downloadLimit);
    dlg->insert(new TStaticText(TRect(39, 3, 44, 4), tr(Str::UnitKBs)));

    fields.globalUploadLimit = addLimitField(dlg, TRect(28, 4, 38, 5), sessionLimits.uploadLimit);
    dlg->insert(new TStaticText(TRect(39, 4, 44, 5), tr(Str::UnitKBs)));

    // --- "Speed Limit" (alt-speed / turtle) mode ---
    dlg->insert(new TStaticText(TRect(2, 7, 56, 8), tr(Str::LabelAltSpeedSection)));

    fields.altSpeedEnabledCheckbox = new TCheckBoxes(TRect(2, 8, 40, 9),
        new TSItem(tr(Str::CheckAltSpeedEnabled), nullptr));
    ushort altSpeedChecked = sessionLimits.altSpeedEnabled ? 0x01 : 0;
    fields.altSpeedEnabledCheckbox->setData(&altSpeedChecked);
    dlg->insert(fields.altSpeedEnabledCheckbox);

    // 2 rows tall: the translated text embeds a '\n' at a natural break
    // point for each language (see Strings.cpp) rather than relying on
    // TStaticText to wrap on its own.
    dlg->insert(new TStaticText(TRect(2, 9, 56, 11), tr(Str::LabelAltSpeedDescription)));

    dlg->insert(new TStaticText(TRect(2, 12, 24, 13), tr(Str::LabelAltSpeedDownload)));
    fields.altSpeedDownloadLimit = addLimitField(dlg, TRect(28, 12, 38, 13), sessionLimits.altSpeedDown);
    dlg->insert(new TStaticText(TRect(39, 12, 44, 13), tr(Str::UnitKBs)));

    dlg->insert(new TStaticText(TRect(2, 13, 24, 14), tr(Str::LabelAltSpeedUpload)));
    fields.altSpeedUploadLimit = addLimitField(dlg, TRect(28, 13, 38, 14), sessionLimits.altSpeedUp);
    dlg->insert(new TStaticText(TRect(39, 13, 44, 14), tr(Str::UnitKBs)));

    // --- Network: on-demand daemon-side checks, not persisted settings
    // — see TransmissionClient::testPort()/updateBlocklist() for why
    // there's nothing here to pre-fill or save back; each button's own
    // result appears in the label right next to it, inline, the moment
    // that one blocking call finishes (see ServerSettingsDialogImpl's
    // own comment on why a popup wasn't used instead).
    dlg->insert(new TStaticText(TRect(2, 15, 56, 16), tr(Str::LabelNetworkSection)));

    dlg->insert(new TButton(TRect(2, 16, 20, 18), tr(Str::ButtonTestPort), cmTestPort, bfNormal));
    auto* portLabel = new TResultLabel(TRect(22, 16, 56, 17), "");
    dlg->insert(portLabel);
    static_cast<ServerSettingsDialogImpl*>(dlg)->portLabel = portLabel;

    dlg->insert(new TButton(TRect(2, 19, 20, 21), tr(Str::ButtonUpdateBlocklist), cmUpdateBlocklist, bfNormal));
    auto* blocklistLabel = new TResultLabel(TRect(22, 19, 56, 20), "");
    dlg->insert(blocklistLabel);
    static_cast<ServerSettingsDialogImpl*>(dlg)->blocklistLabel = blocklistLabel;

    dlg->insert(new TButton(TRect(20, 22, 30, 24), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(32, 22, 42, 24), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

SessionLimits serverSettingsDialogResult(const ServerSettingsDialogFields& fields) {
    SessionLimits limits;
    if (fields.globalLimitCheckboxes) {
        ushort checked = 0;
        fields.globalLimitCheckboxes->getData(&checked);
        limits.downloadLimited = (checked & 0x01) != 0;
        limits.uploadLimited = (checked & 0x02) != 0;
    }
    char buf[32];
    if (fields.globalDownloadLimit) {
        fields.globalDownloadLimit->getData(buf);
        limits.downloadLimit = std::atoi(buf);
    }
    if (fields.globalUploadLimit) {
        fields.globalUploadLimit->getData(buf);
        limits.uploadLimit = std::atoi(buf);
    }
    if (fields.altSpeedEnabledCheckbox) {
        ushort checked = 0;
        fields.altSpeedEnabledCheckbox->getData(&checked);
        limits.altSpeedEnabled = (checked & 0x01) != 0;
    }
    if (fields.altSpeedDownloadLimit) {
        fields.altSpeedDownloadLimit->getData(buf);
        limits.altSpeedDown = std::atoi(buf);
    }
    if (fields.altSpeedUploadLimit) {
        fields.altSpeedUploadLimit->getData(buf);
        limits.altSpeedUp = std::atoi(buf);
    }
    return limits;
}
