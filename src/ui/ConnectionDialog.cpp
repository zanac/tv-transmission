#include "ConnectionDialog.h"
#include "Strings.h"
#include "PasswordInputLine.h"
#include "../rpc/TransmissionClient.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TValidator
#define Uses_TRangeValidator
#define Uses_MsgBox
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

// Local command for the Save/OK button — deliberately NOT cmOK.
// tvision's own TButton/TDialog machinery recognizes cmOK/cmCancel/
// cmYes/cmNo specially (auto-validating every field, then calling
// endModal() directly, before this dialog's own handleEvent() ever
// sees the click) — exactly the shortcut this button can no longer
// take, since it needs to run a connection test (and, on success,
// EITHER save-and-stay-open OR close, depending on its own current
// state — see ConnectionDialogImpl::setDirty()) before either of those
// can happen. Handled entirely by ConnectionDialogImpl::handleEvent()
// below. 62, grouped with TComboBox's own cmComboBoxItemAdded/
// cmComboBoxItemRemoved (60/61, see TComboBox.h) rather than up near
// 100 — App.h's own commands (cmAddTorrent and up) start exactly
// there, so this stays clear of them.
constexpr ushort cmTestConnectionAndOK = 62;

// Same reasoning as TorrentListWindow.cpp's own formatMessage() (not
// shared from there — this file has no other dependency on it): `fmt`
// is one of our own tr() strings with a single "%s" placeholder,
// `value` is a plain argument to it, not itself interpreted as a
// format string.
std::string formatMessage(const char* fmt, const std::string& value) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), fmt, value.c_str());
    return buf;
}

TInputLine* addField(TDialog* dlg, int y, const char* label,
                      const std::string& initialValue, int maxLen,
                      bool masked = false) {
    dlg->insert(new TStaticText(TRect(2, y, 24, y + 1), label));
    TInputLine* input = masked
        ? static_cast<TInputLine*>(new PasswordInputLine(TRect(24, y, 50, y + 1), maxLen))
        : new TInputLine(TRect(24, y, 50, y + 1), maxLen);

    // setData() does memcpy(data, rec, maxLen) on the buffer passed in:
    // it must be at least maxLen readable bytes long, so no direct
    // c_str() of a shorter string.
    std::vector<char> buf(maxLen + 1, 0);
    std::snprintf(buf.data(), buf.size(), "%s", initialValue.c_str());
    input->setData(buf.data());

    dlg->insert(input);
    return input;
}

// Same safety note as addField()'s own buf above — setData() always
// reads dataSize()-1 bytes from whatever's passed in (see
// tinputli.cpp), regardless of how short the actual string is. 256 is
// comfortably more than any field in this dialog's own maxLen (128 at
// most), so this one buffer size covers all of them.
void setFieldText(TInputLine* field, const std::string& value) {
    if (!field) return;
    std::vector<char> buf(256, 0);
    std::snprintf(buf.data(), buf.size(), "%s", value.c_str());
    field->setData(buf.data());
    field->drawView();
}

std::string getFieldText(TInputLine* field) {
    if (!field) return "";
    char buf[256];
    field->getData(buf);
    return buf;
}

// Fills host/port/user/password with `profile`'s own values — used both
// for a name that matches an existing saved server (its real profile)
// and for one that doesn't (a default-constructed ServerProfile, i.e.
// 127.0.0.1:9091 with no user/password) — same helper either way, the
// caller just picks which profile to pass.
void setConnectionFields(ConnectionDialogFields& fields, const ServerProfile& profile) {
    setFieldText(fields.host, profile.host);
    setFieldText(fields.port, std::to_string(profile.port));
    setFieldText(fields.user, profile.user);
    setFieldText(fields.password, profile.password);
}

// Builds the server-name combo's item chain from `current.servers`'
// own keys (alphabetical — see AppSettings.h's own comment on why
// servers is a std::map), and works out which one to focus initially:
// current.activeServer if it's actually one of them, otherwise
// whichever ends up first.
TComboItem* buildServerItems(const AppSettings& current, short& focusedIndex) {
    TComboItem* head = nullptr;
    TComboItem* tail = nullptr;
    focusedIndex = 0;
    short idx = 0;
    for (const auto& [name, profile] : current.servers) {
        (void)profile;
        TComboItem* item = new TComboItem(name.c_str(), 0, nullptr);
        if (name == current.activeServer) focusedIndex = idx;
        if (head == nullptr) head = tail = item;
        else { tail->next = item; tail = item; }
        idx++;
    }
    return head;
}

// A plain TDialog everywhere else in this file's own instantiation
// (`new TDialog(...)`) would have done — subclassed for the reasons
// handled in the one handleEvent() override below: reacting to the
// server-name combo's own cmComboBoxSelectionChanged broadcast (now
// broadened — see TComboBox.h — to cover typing as well as picking
// from the dropdown or "[+]"/"[-]"), showing a confirmation for its
// cmComboBoxItemAdded/cmComboBoxItemRemoved broadcasts, tracking edits
// to the connection fields themselves, and running a connection test
// before Save/OK does anything permanent (see cmTestConnectionAndOK
// above). Kept local to this file rather than given its own header:
// nothing outside createConnectionDialog() below ever needs to name
// this type.
class ConnectionDialogImpl : public TDialog {
public:
    ConnectionDialogImpl(const TRect& r, const char* title) noexcept :
        TWindowInit(&TDialog::initFrame), TDialog(r, title) {}

    ConnectionDialogFields* fields = nullptr;
    const AppSettings* currentSettings = nullptr;
    ServerRemovedCallback onServerRemoved;
    ServerSavedCallback onServerSaved;
    TButton* saveOkButton = nullptr;
    // Starts true unless createConnectionDialog() finds the initially-
    // shown server already has a saved, matching profile (see its own
    // dirty_ initialization) — whether the button reads "Save" (true)
    // or "OK" (false) right now, and so whether clicking it needs to
    // run a fresh connection test or can just close.
    bool dirty_ = true;

    // Relabels the Save/OK button and updates dirty_ together, so the
    // two can never drift apart — every place in this class that
    // changes one always goes through here rather than touching
    // dirty_ directly.
    void setDirty(bool dirty) {
        dirty_ = dirty;
        if (saveOkButton) {
            delete[] (char*)saveOkButton->title; // same alloc/free convention as TWindow's own title — see twindow.cpp/tbutton.cpp
            saveOkButton->title = newStr(dirty ? tr(Str::ButtonSave) : tr(Str::ButtonOK));
            saveOkButton->drawView();
        }
    }

    void handleEvent(TEvent& event) override {
        // Captured BEFORE dispatch: TDialog::handleEvent() below may
        // move focus on its own (e.g. Tab), so `current` right after
        // it no longer reliably says which field an evKeyDown was
        // actually delivered to.
        TView* focusedBefore = current;
        TDialog::handleEvent(event);

        if (event.what == evKeyDown && fields != nullptr &&
            (focusedBefore == fields->host || focusedBefore == fields->port ||
             focusedBefore == fields->user || focusedBefore == fields->password)) {
            // Any direct edit to the connection fields themselves (not
            // just switching servers via the combo) invalidates
            // whatever Save last tested — matches the combo's own
            // cmComboBoxSelectionChanged handling below, just for a
            // different way the shown connection details can change.
            setDirty(true);
        }

        if (event.what == evBroadcast && fields != nullptr &&
            event.message.infoPtr == fields->serverName) {
            if (event.message.command == cmComboBoxSelectionChanged) {
                std::string name = fields->serverName->editText();
                auto it = currentSettings->servers.find(name);
                if (it != currentSettings->servers.end()) {
                    // Matches an already-saved server: load its own
                    // details, and nothing new needs testing.
                    setConnectionFields(*fields, it->second);
                    setDirty(false);
                } else {
                    // New or not-yet-saved name (typed fresh, or picked
                    // right after "[+]" added it) — resets to generic
                    // defaults rather than leaving whatever the
                    // PREVIOUSLY shown server's own details were, which
                    // aren't this one's. Same defaults a brand new
                    // AppSettings::servers entry would start from (see
                    // ServerProfile's own field defaults).
                    setConnectionFields(*fields, ServerProfile{});
                    setDirty(true);
                }
            } else if (event.message.command == cmComboBoxItemAdded) {
                messageBox(formatMessage(tr(Str::MsgServerAdded), fields->serverName->lastChangedValue()),
                           mfInformation | mfOKButton);
            } else if (event.message.command == cmComboBoxItemRemoved) {
                std::string removedName = fields->serverName->lastChangedValue();
                messageBox(formatMessage(tr(Str::MsgServerRemoved), removedName),
                           mfInformation | mfOKButton);
                // Immediate, not gated behind this dialog's own OK/Cancel
                // — see ServerRemovedCallback's own doc comment in
                // ConnectionDialog.h for why "[-]" can't wait for either.
                if (onServerRemoved) onServerRemoved(removedName);
            }
        }

        if (event.what == evCommand && event.message.command == cmTestConnectionAndOK) {
            // Runs every field's own TValidator (the refresh interval
            // and port's TRangeValidator — see createConnectionDialog()
            // below) before even attempting anything — the same check a
            // plain cmOK button gets automatically from tvision itself,
            // which this button no longer does simply by being cmOK.
            // valid()'s own argument only matters for telling cmCancel
            // apart from everything else (cmCancel skips validation
            // entirely) — cmOK here is just conventional, not literally
            // what confirming will end up doing next.
            if (valid(cmOK)) {
                if (!dirty_) {
                    // Already saved and nothing's changed since — Save
                    // already did everything that needed doing (see
                    // onServerSaved below), so this click just closes.
                    endModal(cmOK);
                } else {
                    std::string name = fields->serverName->editText();
                    if (!name.empty()) {
                        ServerProfile profile;
                        profile.host = getFieldText(fields->host);
                        profile.port = std::atoi(getFieldText(fields->port).c_str());
                        profile.user = getFieldText(fields->user);
                        profile.password = getFieldText(fields->password);

                        // A throwaway client, built straight from
                        // what's currently in the fields — never the
                        // app's own shared one for this server (if any
                        // even exists yet), which should stay pointed
                        // at whatever last actually worked until this
                        // one succeeds too.
                        TransmissionClient testClient(profile.host, profile.port,
                                                       profile.user, profile.password);
                        bool ok = false;
                        testClient.getSessionLimits(&ok);
                        if (ok) {
                            // The real save happens first — onServerSaved
                            // persists it and opens/updates the window —
                            // and only then does the combo's own list get
                            // synced to match (addCurrentValue(), a no-op
                            // if the name's already there, e.g. re-Saving
                            // an existing server after editing it — see
                            // its own comment for why this can also show
                            // a "Server added" confirmation the first
                            // time a brand new name is Saved directly,
                            // without "[+]" ever being clicked first).
                            // Cosmetic list-sync last, on purpose: it's
                            // not what actually needs to succeed here.
                            if (onServerSaved) onServerSaved(name, profile);
                            fields->serverName->addCurrentValue();
                            // Stays open — Save is not a close action,
                            // unlike a plain OK; the user may want to
                            // configure another server next, or just
                            // click this same button again (now
                            // reading "OK") to close.
                            setDirty(false);
                        } else {
                            messageBox(formatMessage(tr(Str::MsgConnectionTestFailed), testClient.lastError()),
                                       mfError | mfOKButton);
                        }
                    }
                }
            }
            clearEvent(event);
        }
    }
};

} // namespace

TDialog* createConnectionDialog(const AppSettings& current, ConnectionDialogFields& fields,
                                 ServerRemovedCallback onServerRemoved,
                                 ServerSavedCallback onServerSaved) {
    TRect r(0, 0, 60, 20);
    auto* dlg = new ConnectionDialogImpl(r, tr(Str::DialogTitleConnection));
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 24, 3), tr(Str::LabelLanguage)));
    fields.language = new LanguageComboBox(TRect(24, 2, 50, 3), current.language);
    dlg->insert(fields.language);

    dlg->insert(new TStaticText(TRect(2, 4, 24, 5), tr(Str::LabelServerName)));
    short focusedIndex = 0;
    TComboItem* serverItems = buildServerItems(current, focusedIndex);
    fields.serverName = new TComboBox(TRect(24, 4, 50, 5), serverItems, focusedIndex);
    fields.serverName->setEditable(true);

    // Which name ends up shown initially, and whether it already
    // matches a saved profile — decides both the connection fields'
    // own starting values and the Save/OK button's own starting label
    // (see the dirty_ assignment further below), all in one place
    // rather than recomputing "is this name already saved?" twice.
    std::string initialName = tr(Str::DefaultServerName);
    bool initialMatch = false;
    if (serverItems == nullptr) {
        // No servers configured yet (a fresh install) — nothing for
        // the combo to focus, so its own editText() would otherwise
        // start out empty with no hint of what to type. Not saved
        // anywhere until Save (or "[+]") is used.
        fields.serverName->setEditText(initialName);
    } else {
        initialName = fields.serverName->editText();
        initialMatch = current.servers.find(initialName) != current.servers.end();
    }
    dlg->insert(fields.serverName);

    fields.refreshInterval = addField(dlg, 6, tr(Str::LabelRefreshSeconds),
        std::to_string(current.refreshIntervalSeconds), 10);
    // TRangeValidator filters non-digit keystrokes as they're typed (see
    // isValidInput() in tvision's tvalidat.cpp) and blocks confirming the
    // dialog with an out-of-range value (shows its own "Value not in the
    // range X to Y" messageBox) — real validation, not just parsing
    // whatever ends up in the field after the fact.
    fields.refreshInterval->setValidator(new TRangeValidator(1, 86400)); // up to 24h
    // Starting values match whichever server ended up initially shown
    // above (its own saved profile if it has one, otherwise a
    // default-constructed ServerProfile — same "no match → defaults"
    // rule setConnectionFields()/cmComboBoxSelectionChanged use
    // everywhere else in this dialog).
    ServerProfile initialProfile = initialMatch ? current.servers.at(initialName) : ServerProfile{};
    fields.host = addField(dlg, 8, tr(Str::LabelHost), initialProfile.host, 128);
    fields.port = addField(dlg, 10, tr(Str::LabelPort), std::to_string(initialProfile.port), 10);
    fields.port->setValidator(new TRangeValidator(1, 65535)); // valid TCP port range
    fields.user = addField(dlg, 12, tr(Str::LabelUser), initialProfile.user, 128);
    fields.password = addField(dlg, 14, tr(Str::LabelPassword), initialProfile.password, 128, /*masked=*/true);

    auto* impl = static_cast<ConnectionDialogImpl*>(dlg);
    impl->fields = &fields;
    impl->currentSettings = &current;
    impl->onServerRemoved = std::move(onServerRemoved);
    impl->onServerSaved = std::move(onServerSaved);
    impl->dirty_ = !initialMatch;

    // A blank row (16) between the last field and the buttons, rather
    // than the buttons sitting immediately under Password. The
    // Save/OK button uses cmTestConnectionAndOK, not cmOK — see its
    // own definition above for why — but keeps bfDefault (Enter still
    // reaches it the same way a real cmOK button would). Starting
    // label matches impl->dirty_ (just set above): "Save" for a new/
    // unmatched server, "OK" for one that's already saved as-is.
    auto* button = new TButton(TRect(20, 17, 30, 19),
        tr(impl->dirty_ ? Str::ButtonSave : Str::ButtonOK), cmTestConnectionAndOK, bfDefault);
    impl->saveOkButton = button;
    dlg->insert(button);
    dlg->insert(new TButton(TRect(32, 17, 42, 19), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

AppSettings connectionDialogResult(const ConnectionDialogFields& fields, const AppSettings& current) {
    AppSettings result = current;

    if (fields.refreshInterval) {
        char buf[256];
        fields.refreshInterval->getData(buf);
        int v = std::atoi(buf);
        if (v > 0) result.refreshIntervalSeconds = v;
    }
    if (fields.language) {
        result.language = fields.language->language();
    }

    return result;
}
