#include "TrackerListWindow.h"
#include "TrackerDetailWindow.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TProgram
#include <tvision/tv.h>
#include <cstdio>
#include <cstring>

namespace {

// Column widths shared between the header and getText(), same
// "constants instead of hand-counting" approach as the main torrent
// list (see TorrentListWindow.cpp) — cheaper to get right here since
// there's no click-to-sort math depending on them, just alignment.
constexpr int kHostW = 30;
constexpr int kTierW = 8;   // "Tier 10" (7 chars) + 1
constexpr int kSeedersW = 8;
constexpr int kLeechersW = 8;
constexpr int kDownloadedW = 10;
constexpr int kStatusW = 8; // "Errore" (6, longest translation) + 2

std::string rightAlign(const std::string& s, size_t width) {
    if (s.size() >= width) return s;
    return std::string(width - s.size(), ' ') + s;
}

// Transmission uses -1 for "not known yet" (e.g. before the first
// successful announce) — show "N/A" rather than a misleading number.
std::string formatCount(int v) {
    if (v < 0) return tr(Str::ValueNotAvailable);
    return std::to_string(v);
}

std::string buildHeaderText() {
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%-*s %*s %*s %*s %*s  %s",
        kHostW, tr(Str::HeaderTrackerHost),
        kTierW, tr(Str::HeaderTier),
        kSeedersW, tr(Str::HeaderSeeders),
        kLeechersW, tr(Str::HeaderLeechers),
        kDownloadedW, tr(Str::HeaderDownloaded),
        tr(Str::HeaderTrackerStatus));
    return buf;
}

// Local to this window: scoped to its own handleEvent (same reasoning
// as TorrentDetailsWindow's cmApplySpeedLimits/cmCloseDetails).
constexpr ushort cmRefreshTrackers = 210;
constexpr ushort cmCloseTrackers = 211;

} // namespace

TrackerListViewer::TrackerListViewer(const TRect& r, TScrollBar* vScrollBar)
    : TListViewer(r, 1, nullptr, vScrollBar) {
    setRange(0);
}

void TrackerListViewer::setTrackers(std::vector<TrackerStat> trackers) {
    trackers_ = std::move(trackers);
    setRange((short)trackers_.size());
    if (focused >= range && range > 0) focusItem(range - 1);
    drawView();
}

const TrackerStat* TrackerListViewer::selectedTracker() const {
    if (focused < 0 || focused >= (int)trackers_.size()) return nullptr;
    return &trackers_[focused];
}

void TrackerListViewer::getText(char* dest, short item, short maxLen) {
    if (item < 0 || item >= (int)trackers_.size()) {
        dest[0] = '\0';
        return;
    }
    const TrackerStat& t = trackers_[item];
    std::string host = padOrTruncateUtf8(t.host, kHostW);
    std::string tierStr = rightAlign("Tier " + std::to_string(t.tier + 1), kTierW);
    std::string seeders = rightAlign(formatCount(t.seederCount), kSeedersW);
    std::string leechers = rightAlign(formatCount(t.leecherCount), kLeechersW);
    std::string downloaded = rightAlign(formatCount(t.downloadCount), kDownloadedW);
    const char* status = !t.hasAnnounced ? "" :
        (t.lastAnnounceSucceeded ? tr(Str::TrackerStatusOk) : tr(Str::TrackerStatusError));
    std::snprintf(dest, maxLen, "%s %s %s %s %s  %s",
        host.c_str(), tierStr.c_str(), seeders.c_str(), leechers.c_str(),
        downloaded.c_str(), status);
}

TrackerListWindow::TrackerListWindow(const TRect& bounds, TStringView title,
                                      int torrentId, TransmissionClient& client)
    : TWindowInit(&TDialog::initFrame),
      TDialog(bounds, title),
      torrentId_(torrentId), client_(client) {
    options |= ofCentered;

    TRect r = getExtent();
    r.grow(-1, -1);

    TRect headerRect(r.a.x, r.a.y, r.b.x, r.a.y + 1);
    r.a.y += 1;
    r.b.y -= 3; // room for the button row at the bottom

    TRect scrollRect(r.b.x - 1, r.a.y, r.b.x, r.b.y);
    TRect listRect(r.a.x, r.a.y, r.b.x - 1, r.b.y);

    TScrollBar* vScroll = new TScrollBar(scrollRect);
    insert(vScroll);

    listViewer_ = new TrackerListViewer(listRect, vScroll);
    insert(listViewer_);

    insert(new TStaticText(headerRect, buildHeaderText().c_str()));

    int buttonY = r.b.y + 1;
    insert(new TButton(TRect(r.a.x + 10, buttonY, r.a.x + 22, buttonY + 2),
        tr(Str::ButtonRefresh), cmRefreshTrackers, bfDefault));
    insert(new TButton(TRect(r.a.x + 26, buttonY, r.a.x + 38, buttonY + 2),
        tr(Str::ButtonClose), cmCloseTrackers, bfNormal));

    refresh();
}

void TrackerListWindow::refresh() {
    listViewer_->setTrackers(client_.getTrackerStats(torrentId_));
}

void TrackerListWindow::showDetailForSelected() {
    if (const TrackerStat* t = listViewer_->selectedTracker())
        if (auto* win = createTrackerDetailWindow(*t))
            TProgram::application->insertWindow(win);
}

void TrackerListWindow::handleEvent(TEvent& event) {
    TDialog::handleEvent(event);
    if (event.what == evCommand && event.message.command == cmRefreshTrackers) {
        refresh();
        clearEvent(event);
    } else if (event.what == evCommand && event.message.command == cmCloseTrackers) {
        close();
        clearEvent(event);
    } else if (event.what == evBroadcast &&
               event.message.command == cmListItemSelected &&
               event.message.infoPtr == listViewer_) {
        showDetailForSelected();
        clearEvent(event);
    }
}

TDialog* createTrackerListWindow(int torrentId, const std::string& torrentName,
                                  TransmissionClient& client) {
    TRect r(0, 0, 76, 20);
    std::string shortName = truncateUtf8(torrentName, 30);
    char titleBuf[128];
    std::snprintf(titleBuf, sizeof(titleBuf), tr(Str::WindowTitleTrackerList), shortName.c_str());
    return new TrackerListWindow(r, titleBuf, torrentId, client);
}
