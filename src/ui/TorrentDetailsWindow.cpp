#include "TorrentDetailsWindow.h"
#include "TrackerListWindow.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TStaticText
#define Uses_TButton
#define Uses_TSItem
#define Uses_TEvent
#define Uses_TValidator
#define Uses_TRangeValidator
#define Uses_TProgram
#include <tvision/tv.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

// Local to this window: scoped to its own handleEvent, so it doesn't
// need to avoid App.h's cmXxx range (100-106) — a command value is only
// ever ambiguous within the single handleEvent that reacts to it.
constexpr ushort cmApplySpeedLimits = 200;

// NOT cmClose: TButton::press() sets event.message.infoPtr to the
// button itself (see tbutton.cpp), but TWindow::handleEvent only acts
// on cmClose when infoPtr is 0 or the window itself — a button-sent
// cmClose is silently ignored by the base class. Using a dedicated
// command and calling close() directly (below) sidesteps that check
// entirely; it's the same close() TWindow::handleEvent would have
// called anyway.
constexpr ushort cmCloseDetails = 201;
constexpr ushort cmShowTrackers = 202;

// TStaticText already copies the text internally (newStr + delete[] in
// its destructor), so passing it the temporary buffer is enough: there's
// no need (and it would leak memory) to duplicate it here.
void addLine(TWindow* win, int y, const char* text) {
    TRect r(2, y, 56, y + 1);
    win->insert(new TStaticText(r, text));
}

} // namespace

TorrentDetailsWindow::TorrentDetailsWindow(const TRect& bounds, TStringView title,
                                            int torrentId, const std::string& torrentName,
                                            TransmissionClient& client)
    : TWindowInit(&TDialog::initFrame),
      TDialog(bounds, title),
      torrentId_(torrentId), torrentName_(torrentName), client_(client) {}

void TorrentDetailsWindow::showTrackers() {
    if (auto* win = createTrackerListWindow(torrentId_, torrentName_, client_))
        TProgram::application->insertWindow(win);
}

void TorrentDetailsWindow::applySpeedLimits() {
    // Unlike a modal dialog closed via execView() (where an attached
    // TValidator blocks cmOK automatically), this window's "Apply"
    // button is handled directly by us — so the validator's range check
    // needs to be triggered explicitly here. valid(cmOK) both runs it
    // and shows the validator's own error messageBox if it fails.
    if (!downloadLimitField->valid(cmOK) || !uploadLimitField->valid(cmOK))
        return;

    ushort checked = 0;
    limitCheckboxes->getData(&checked);
    bool downloadLimited = (checked & 0x01) != 0;
    bool uploadLimited = (checked & 0x02) != 0;
    bool honorsSessionLimits = (checked & 0x04) != 0;

    char buf[32];
    downloadLimitField->getData(buf);
    int downloadLimit = std::atoi(buf);
    uploadLimitField->getData(buf);
    int uploadLimit = std::atoi(buf);

    client_.setTorrentSpeedLimits(torrentId_, downloadLimited, downloadLimit,
                                   uploadLimited, uploadLimit, honorsSessionLimits);
}

void TorrentDetailsWindow::handleEvent(TEvent& event) {
    TDialog::handleEvent(event);
    if (event.what == evCommand && event.message.command == cmApplySpeedLimits) {
        applySpeedLimits();
        clearEvent(event);
    } else if (event.what == evCommand && event.message.command == cmCloseDetails) {
        close();
        clearEvent(event);
    } else if (event.what == evCommand && event.message.command == cmShowTrackers) {
        showTrackers();
        clearEvent(event);
    }
}

TWindow* createTorrentDetailsWindow(const Torrent& t, TransmissionClient& client) {
    TRect r(0, 0, 58, 20);

    // Title includes the start of the torrent's name, so several open
    // details windows (see the "Window list" dialog) are distinguishable
    // at a glance instead of all reading "Torrent details".
    char titleBuf[128];
    std::string shortName = truncateUtf8(t.name, 30);
    std::snprintf(titleBuf, sizeof(titleBuf), "%s: %s",
        tr(Str::WindowTitleDetails), shortName.c_str());

    auto* win = new TorrentDetailsWindow(r, titleBuf, t.id, t.name, client);
    win->options |= ofCentered;

    char buf[256];

    std::snprintf(buf, sizeof(buf), tr(Str::LabelName), t.name.c_str());
    addLine(win, 2, buf);

    std::string sizeStr = formatSize(t.sizeBytes);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelSize), sizeStr.c_str());
    addLine(win, 3, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelCompleted), t.percentDone * 100.0);
    addLine(win, 4, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelDownload), t.rateDownload / 1024.0);
    addLine(win, 5, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelUpload), t.rateUpload / 1024.0);
    addLine(win, 6, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelStatus), trTorrentStatus(t.status));
    addLine(win, 7, buf);

    std::string addedStr = formatUnixTimestamp(t.addedDate);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelAdded), addedStr.c_str());
    addLine(win, 8, buf);

    if (!t.errorString.empty()) {
        std::snprintf(buf, sizeof(buf), tr(Str::LabelError), t.errorString.c_str());
        addLine(win, 9, buf);
    }

    std::snprintf(buf, sizeof(buf), tr(Str::LabelId), t.id);
    addLine(win, 10, buf);

    // --- Per-torrent speed limit override ---
    addLine(win, 12, tr(Str::LabelSpeedLimitSection));

    // Three independent checkboxes, NOT two + an implied third state:
    // "limit download/upload" (this torrent's own cap) and "honor global
    // speed limits" (whether it follows the session-wide limit at all)
    // are separate Transmission flags — see the comment on
    // Torrent::honorsSessionLimits in Torrent.h for why one can't be
    // inferred from the other.
    //
    // Width: TCluster draws each item as "[ ] " (occupying 5 columns)
    // followed by its label (see drawMultiBox() in tvision's
    // tcluster.cpp) — the rect must be at least 5 + the longest label's
    // length, or the label gets silently cut off. "Honor global speed
    // limits" is 25 characters, so 5+25=30 is the bare minimum; this
    // uses 36 for some breathing room.
    win->limitCheckboxes = new TCheckBoxes(TRect(2, 13, 38, 16),
        new TSItem(tr(Str::CheckLimitDownload),
        new TSItem(tr(Str::CheckLimitUpload),
        new TSItem(tr(Str::CheckHonorGlobalLimits), nullptr))));
    ushort checked = (t.downloadLimited ? 0x01 : 0) | (t.uploadLimited ? 0x02 : 0) |
                     (t.honorsSessionLimits ? 0x04 : 0);
    win->limitCheckboxes->setData(&checked);
    win->insert(win->limitCheckboxes);

    win->downloadLimitField = new TInputLine(TRect(40, 13, 50, 14), 8);
    std::vector<char> downloadBuf(9, 0);
    std::snprintf(downloadBuf.data(), downloadBuf.size(), "%d", t.downloadLimit);
    win->downloadLimitField->setData(downloadBuf.data());
    win->downloadLimitField->setValidator(new TRangeValidator(0, 1000000)); // KB/s, ~1GB/s cap
    win->insert(win->downloadLimitField);
    win->insert(new TStaticText(TRect(51, 13, 56, 14), tr(Str::UnitKBs)));

    win->uploadLimitField = new TInputLine(TRect(40, 14, 50, 15), 8);
    std::vector<char> uploadBuf(9, 0);
    std::snprintf(uploadBuf.data(), uploadBuf.size(), "%d", t.uploadLimit);
    win->uploadLimitField->setData(uploadBuf.data());
    win->uploadLimitField->setValidator(new TRangeValidator(0, 1000000));
    win->insert(win->uploadLimitField);
    win->insert(new TStaticText(TRect(51, 14, 56, 15), tr(Str::UnitKBs)));

    win->insert(new TButton(TRect(14, 17, 24, 19), tr(Str::ButtonApply), cmApplySpeedLimits, bfDefault));
    win->insert(new TButton(TRect(28, 17, 38, 19), tr(Str::ButtonClose), cmCloseDetails, bfNormal));
    win->insert(new TButton(TRect(42, 17, 54, 19), tr(Str::ButtonTrackers), cmShowTrackers, bfNormal));

    return win;
}
