#include "TorrentListWindow.h"
#include "TorrentDetailsWindow.h"
#include "TorrentFilesWindow.h"
#include "Strings.h"
#include "../TextUtil.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TMenuItem
#define Uses_TMenu
#define Uses_TMenuPopup
#define Uses_MsgBox
#define Uses_TKeys
#include <tvision/tv.h>
#include "App.h"

namespace {

// Builds "[███████░░░░░░░░░]  42%" — the block characters (U+2588 full
// block / U+2591 light shade) are a near-universal convention for
// filled/empty progress in any UTF-8 terminal; the count of each is
// exactly proportional to percentDone, so the bar visibly fills up as
// the torrent approaches 100%.
constexpr int kBarInnerW = 16;
std::string buildProgressBar(double percentDone) {
    int filled = static_cast<int>(std::lround(percentDone * kBarInnerW));
    if (filled < 0) filled = 0;
    if (filled > kBarInnerW) filled = kBarInnerW;
    std::string bar = "[";
    for (int i = 0; i < filled; i++) bar += "\u2588";
    for (int i = 0; i < kBarInnerW - filled; i++) bar += "\u2591";
    bar += "]";
    char pct[8];
    std::snprintf(pct, sizeof(pct), " %3.0f%%", percentDone * 100.0);
    bar += pct;
    return bar;
}

// Transmission's own sentinels for uploadRatio: -1 = not available (e.g.
// no data transferred yet), -2 = infinite (uploaded something with
// nothing downloaded, e.g. a torrent added as a seed). Shown as "—"/"∞"
// rather than the literal negative number, which would look like an
// error rather than a special case.
std::string formatRatio(double ratio) {
    if (ratio <= -2.0) return "\xE2\x88\x9E";    // ∞
    if (ratio < 0.0) return "\xE2\x80\x94";       // —
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.2f", ratio);
    return buf;
}

// Transmission's own sentinels for eta: -1 = not available, -2 =
// unknown — both shown as "—" rather than a negative second count.
std::string formatEta(int64_t etaSeconds) {
    if (etaSeconds < 0) return "\xE2\x80\x94"; // —
    int64_t hours = etaSeconds / 3600;
    int64_t minutes = (etaSeconds % 3600) / 60;
    char buf[32];
    if (hours > 0) std::snprintf(buf, sizeof(buf), "%lldh %lldm", (long long)hours, (long long)minutes);
    else std::snprintf(buf, sizeof(buf), "%lldm", (long long)minutes);
    return buf;
}

std::string formatPriority(int priority) {
    if (priority < 0) return tr(Str::PriorityLow);
    if (priority > 0) return tr(Str::PriorityHigh);
    return tr(Str::PriorityNormal);
}

// tr_torrent_activity values (Transmission RPC): 0=stopped,
// 1=check-wait, 2=checking, 3=download-wait, 4=downloading,
// 5=seed-wait, 6=seeding.
//
// A row's color reflects the torrent's status at a glance; an error
// takes priority over the status-based color since it's the most
// actionable state to notice.
TColorAttr statusRowColor(const Torrent& t) {
    if (!t.errorString.empty()) return TColorAttr(0x1C); // error: light red on blue
    switch (t.status) {
        case 4: return TColorAttr(0x1B); // downloading: light cyan on blue
        case 6: return TColorAttr(0x1A); // seeding: light green on blue
        case 0: return TColorAttr(0x17); // stopped: light gray on blue (dimmer)
        case 1: case 2: case 3: case 5:
                return TColorAttr(0x1E); // checking/queued: yellow on blue
        default: return TColorAttr(0x1F); // fallback: white on blue
    }
}

// "Queued" here means 1/3/5: already started (waiting for its turn),
// as opposed to genuinely stopped (0).
bool isStopped(const Torrent& t) { return t.status == 0; }
bool isQueued(const Torrent& t) { return t.status == 1 || t.status == 3 || t.status == 5; }

void setCmd(TView* v, ushort cmd, bool enable) {
    if (enable) v->enableCommand(cmd);
    else v->disableCommand(cmd);
}

// `fmt` is one of our own tr() strings with a single "%s" placeholder;
// `value` is a plain argument to it, not itself interpreted as a format
// string, so a torrent name containing a literal '%' can't cause any
// issue here (unlike passing it directly to printf as the format).
std::string formatMessage(const char* fmt, const std::string& value) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), fmt, value.c_str());
    return buf;
}

// Case-insensitive substring search — an empty `needle` (no name filter
// set) always matches, same as the filter not existing.
bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
        [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
    return it != haystack.end();
}

// A torrent is shown only if it satisfies EVERY active filter (AND, not
// OR) — see TorrentFilter's own comment in AppSettings.h.
bool passesFilter(const Torrent& t, const TorrentFilter& f) {
    if (!containsCaseInsensitive(t.name, f.nameContains)) return false;
    switch (t.status) {
        case 0: return f.showStopped;
        case 1: return f.showCheckWait;
        case 2: return f.showChecking;
        case 3: return f.showDownloadWait;
        case 4: return f.showDownloading;
        case 5: return f.showSeedWait;
        case 6: return f.showSeeding;
        default: return true;
    }
}

} // namespace

TorrentListWindow::TorrentListWindow(const TRect& bounds, TransmissionClient& client,
                                      SortColumn initialSort, bool initialAscending,
                                      TorrentFilter initialFilter,
                                      const std::vector<int>& initialColumnWidths,
                                      const std::vector<int>& initialColumnOrder,
                                      const std::vector<bool>& initialColumnVisible,
                                      SortChangedCallback onSortChanged,
                                      const std::vector<int>& initialTrackerColumnWidths,
                                      const std::vector<int>& initialTrackerColumnOrder,
                                      const std::vector<bool>& initialTrackerColumnVisible)
    : TWindowInit(&TWindow::initFrame), // virtual base: must be initialized here,
                                         // by the most-derived class — TGridWindow's
                                         // own initialization of it doesn't propagate
                                         // through another level of inheritance
      TGridWindow(bounds, tr(Str::WindowTitleTorrentList), /*fullScreen=*/true,
                  gvResizableColumns | gvReorderableColumns | gvMultiSelect),
      client_(client),
      initialTrackerColumnWidths_(initialTrackerColumnWidths),
      initialTrackerColumnOrder_(initialTrackerColumnOrder),
      initialTrackerColumnVisible_(initialTrackerColumnVisible),
      filter_(std::move(initialFilter)),
      sortColumn_(initialSort), sortAscending_(initialAscending),
      onSortChanged_(std::move(onSortChanged)) {
    setupColumns(initialColumnWidths);
    applyColumnLabels();
    // Restores a persisted arrangement without needing any reaction from
    // this window: reordering only changes where columns are drawn, not
    // what data they show (see TGridView's logical/visual index split),
    // so unlike the sort indicator just below there's nothing else to
    // keep in sync here.
    grid()->setColumnOrder(initialColumnOrder);
    setColumnVisibility(initialColumnVisible);
    // Reflects the persisted sort in the header's "^"/"v" indicator
    // without going through setSortChangedCallback()'s callback: the
    // data isn't loaded yet (refresh() below sorts it, using
    // sortColumn_/sortAscending_ directly), so there's nothing to
    // re-sort in reaction to this — it would just be redundant.
    grid()->setSortIndicator((int)sortColumn_, sortAscending_);

    grid()->setCellTextCallback([this](int row, int col) -> std::string {
        if (row < 0 || row >= (int)visible_.size()) return "";
        const Torrent& t = visible_[row];
        switch (static_cast<SortColumn>(col)) {
            case SortColumn::Name:   return t.name;
            case SortColumn::Done:   return buildProgressBar(t.percentDone);
            case SortColumn::Size:   return formatSize(t.sizeBytes);
            case SortColumn::Down: {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "D:%7.1fKB/s", t.rateDownload / 1024.0);
                return buf;
            }
            case SortColumn::Up: {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "U:%7.1fKB/s", t.rateUpload / 1024.0);
                return buf;
            }
            case SortColumn::Added:  return formatUnixTimestamp(t.addedDate);
            case SortColumn::Status: return trTorrentStatus(t.status);
            case SortColumn::Ratio:      return formatRatio(t.uploadRatio);
            case SortColumn::Uploaded:   return formatSize(t.uploadedEver);
            case SortColumn::Downloaded: return formatSize(t.downloadedEver);
            case SortColumn::Location:   return t.downloadDir;
            case SortColumn::Eta:        return formatEta(t.eta);
            case SortColumn::Peers:      return std::to_string(t.peersConnected);
            case SortColumn::QueuePosition: return std::to_string(t.queuePosition + 1); // shown 1-based
            case SortColumn::Priority:   return formatPriority(t.bandwidthPriority);
            case SortColumn::CompletedDate:
                return t.doneDate > 0 ? formatUnixTimestamp(t.doneDate) : "\xE2\x80\x94"; // —
        }
        return "";
    });

    grid()->setRowColorCallback([this](int row, bool focused) -> TColorAttr {
        if (focused) return TColorAttr(0xF0); // current row: black on white, regardless of status
        if (row < 0 || row >= (int)visible_.size()) return TColorAttr(0x1F);
        return statusRowColor(visible_[row]);
    });

    // Only the name column is bold — see TGridView's own draw(): applied
    // regardless of focus, same as this window's rendering did before
    // the TGridView migration.
    grid()->setCellBoldCallback([](int, int col) { return col == (int)SortColumn::Name; });

    // Sorting-on-click itself (toggling direction, drawing "^"/"v") is
    // now handled entirely inside TGridView (see its own
    // setSortChangedCallback doc comment) — this callback only needs to
    // react to the choice: re-sort visible_ and let the caller persist
    // it, same as the old toggleSort() did.
    grid()->setSortChangedCallback([this](int col, bool ascending) {
        sortColumn_ = static_cast<SortColumn>(col);
        sortAscending_ = ascending;
        applyFilterAndSort();
        if (onSortChanged_) onSortChanged_(sortColumn_, sortAscending_);
    });
    grid()->setRowActivateCallback([this](int) { showDetailsForSelected(); });
    grid()->setRowContextCallback([this](int row, TPoint pos) { showContextMenuFor(row, pos); });
    grid()->setRowFocusCallback([this](int) { updateCommandStates(); });

    refresh();
}

void TorrentListWindow::setupColumns(const std::vector<int>& initialWidths) {
    grid()->clearColumns();

    TGridColumn name;
    name.header = tr(Str::HeaderName);
    name.width = 30;
    name.minWidth = 10;
    grid()->addColumn(name);

    TGridColumn done;
    done.header = tr(Str::HeaderDone);
    done.width = 23;       // "[" + 16 blocks + "]" + " NNN%" (5) = 23
    done.minWidth = 23;
    // Not resizable: buildProgressBar() always produces a fixed-width
    // 23-character string (a fixed 16-block-wide bar), so shrinking this
    // column would just truncate the bar/percentage rather than
    // reflowing anything — better to not offer a resize that can't do
    // anything useful.
    done.resizable = false;
    grid()->addColumn(done);

    TGridColumn size;
    size.header = tr(Str::HeaderSize);
    size.width = 10;
    size.minWidth = 6;
    size.align = TGridColumn::Align::Right;
    grid()->addColumn(size);

    TGridColumn down;
    down.header = tr(Str::HeaderDownload);
    down.width = 14;
    down.minWidth = 8;
    grid()->addColumn(down);

    TGridColumn up;
    up.header = tr(Str::HeaderUpload);
    up.width = 14;
    up.minWidth = 8;
    grid()->addColumn(up);

    TGridColumn added;
    added.header = tr(Str::HeaderAdded);
    added.width = 17;
    added.minWidth = 10;
    grid()->addColumn(added);

    TGridColumn status;
    status.header = tr(Str::HeaderStatus);
    status.width = 23;
    status.minWidth = 8;
    grid()->addColumn(status);

    // Hidden-by-default columns — useful to some, but not part of the
    // list's default look; shown via "Manage columns...".
    TGridColumn ratio;
    ratio.header = tr(Str::HeaderRatio);
    ratio.width = 8;
    ratio.minWidth = 5;
    ratio.align = TGridColumn::Align::Right;
    ratio.visible = false;
    grid()->addColumn(ratio);

    TGridColumn uploaded;
    uploaded.header = tr(Str::HeaderTotalUploaded);
    uploaded.width = 10;
    uploaded.minWidth = 6;
    uploaded.align = TGridColumn::Align::Right;
    uploaded.visible = false;
    grid()->addColumn(uploaded);

    TGridColumn downloaded;
    downloaded.header = tr(Str::HeaderTotalDownloaded);
    downloaded.width = 10;
    downloaded.minWidth = 6;
    downloaded.align = TGridColumn::Align::Right;
    downloaded.visible = false;
    grid()->addColumn(downloaded);

    TGridColumn location;
    location.header = tr(Str::HeaderLocation);
    location.width = 30;
    location.minWidth = 10;
    location.visible = false;
    grid()->addColumn(location);

    TGridColumn eta;
    eta.header = tr(Str::HeaderEta);
    eta.width = 10;
    eta.minWidth = 6;
    eta.align = TGridColumn::Align::Right;
    eta.visible = false;
    grid()->addColumn(eta);

    TGridColumn peers;
    peers.header = tr(Str::HeaderPeers);
    peers.width = 6;
    peers.minWidth = 4;
    peers.align = TGridColumn::Align::Right;
    peers.visible = false;
    grid()->addColumn(peers);

    TGridColumn queuePos;
    queuePos.header = tr(Str::HeaderQueuePosition);
    queuePos.width = 6;
    queuePos.minWidth = 4;
    queuePos.align = TGridColumn::Align::Right;
    queuePos.visible = false;
    grid()->addColumn(queuePos);

    TGridColumn priority;
    priority.header = tr(Str::HeaderPriority);
    priority.width = 8;
    priority.minWidth = 6;
    priority.visible = false;
    grid()->addColumn(priority);

    TGridColumn completedDate;
    completedDate.header = tr(Str::HeaderCompletedDate);
    completedDate.width = 17;
    completedDate.minWidth = 10;
    completedDate.visible = false;
    grid()->addColumn(completedDate);

    // Persisted widths from a previous session, applied on top of the
    // defaults above — only if there's exactly one per column: a
    // mismatched count (first run with no saved widths yet, or a
    // settings.json from before a column was added/removed) falls back
    // to the defaults just set rather than applying them partially or
    // out of order.
    if (initialWidths.size() == (size_t)grid()->columnCount()) {
        for (int i = 0; i < grid()->columnCount(); i++)
            grid()->setColumnWidth(i, initialWidths[i]);
    }
}

void TorrentListWindow::applyColumnLabels() {
    static const Str kLabels[] = {
        Str::HeaderName, Str::HeaderDone, Str::HeaderSize,
        Str::HeaderDownload, Str::HeaderUpload, Str::HeaderAdded, Str::HeaderStatus,
        Str::HeaderRatio, Str::HeaderTotalUploaded, Str::HeaderTotalDownloaded, Str::HeaderLocation,
        Str::HeaderEta, Str::HeaderPeers, Str::HeaderQueuePosition, Str::HeaderPriority,
        Str::HeaderCompletedDate,
    };
    int n = std::min(grid()->columnCount(), (int)(sizeof(kLabels) / sizeof(kLabels[0])));
    for (int i = 0; i < n; i++) grid()->column(i).header = tr(kLabels[i]);
}

void TorrentListWindow::retranslate() {
    // title is allocated with newStr() by TWindow's constructor (see
    // twindow.cpp) and freed with delete[] in its destructor — the same
    // pattern used for TStatusItem::text in BandwidthStatusLine.
    delete[] (char*)title;
    title = newStr(tr(Str::WindowTitleTorrentList));
    applyColumnLabels(); // the sort "^"/"v" indicator is drawn by TGridView
                         // itself at draw time (see grid()->refresh() below),
                         // independent of the header label text — nothing
                         // to reapply here for it.
    grid()->refresh();
    drawView();
}

void TorrentListWindow::refresh() {
    allTorrents_ = client_.listTorrents();
    applyFilterAndSort();
}

void TorrentListWindow::setFilter(TorrentFilter filter) {
    filter_ = std::move(filter);
    applyFilterAndSort();
}

std::vector<int> TorrentListWindow::columnWidths() const {
    std::vector<int> widths;
    widths.reserve(grid()->columnCount());
    for (int i = 0; i < grid()->columnCount(); i++)
        widths.push_back(grid()->column(i).width);
    return widths;
}

std::vector<int> TorrentListWindow::columnOrder() const {
    return grid()->columnOrder();
}

std::vector<bool> TorrentListWindow::columnVisibility() const {
    std::vector<bool> vis(kTorrentColumnCount, true);
    for (int i = 0; i < kTorrentColumnCount; i++) vis[i] = grid()->isColumnVisible(i);
    return vis;
}

void TorrentListWindow::setColumnVisibility(const std::vector<bool>& visible) {
    for (int i = 0; i < kTorrentColumnCount; i++) {
        // A missing entry falls back to that column's own default —
        // true for the original 7, false for the 9 added later (see
        // setupColumns()) — rather than always true. That matters for
        // a settings.json saved before those 9 existed: it only has 7
        // entries, and without this, the missing 9 would silently come
        // back shown instead of hidden-by-default as intended.
        bool defaultShown = (i < 7);
        bool shown = (i < (int)visible.size()) ? visible[i] : defaultShown;
        grid()->setColumnVisible(i, shown);
    }
}

void TorrentListWindow::applyFilterAndSort() {
    visible_.clear();
    visible_.reserve(allTorrents_.size());
    for (const auto& t : allTorrents_)
        if (passesFilter(t, filter_)) visible_.push_back(t);

    // Always compare "ascending" but with the arguments swapped for
    // descending order, instead of negating the result: negating `less`
    // to get `greater` breaks the strict-weak-ordering std::sort
    // requires when two elements are equal (a<b false AND b<a false, but
    // !less(a,b) would still be "true" both ways).
    std::sort(visible_.begin(), visible_.end(),
        [this](const Torrent& a, const Torrent& b) {
            const Torrent& x = sortAscending_ ? a : b;
            const Torrent& y = sortAscending_ ? b : a;
            switch (sortColumn_) {
                case SortColumn::Name:   return x.name < y.name;
                case SortColumn::Done:   return x.percentDone < y.percentDone;
                case SortColumn::Size:   return x.sizeBytes < y.sizeBytes;
                case SortColumn::Down:   return x.rateDownload < y.rateDownload;
                case SortColumn::Up:     return x.rateUpload < y.rateUpload;
                case SortColumn::Added:  return x.addedDate < y.addedDate;
                case SortColumn::Status: return x.status < y.status;
                case SortColumn::Ratio:         return x.uploadRatio < y.uploadRatio;
                case SortColumn::Uploaded:      return x.uploadedEver < y.uploadedEver;
                case SortColumn::Downloaded:    return x.downloadedEver < y.downloadedEver;
                case SortColumn::Location:      return x.downloadDir < y.downloadDir;
                case SortColumn::Eta:           return x.eta < y.eta;
                case SortColumn::Peers:         return x.peersConnected < y.peersConnected;
                case SortColumn::QueuePosition: return x.queuePosition < y.queuePosition;
                case SortColumn::Priority:      return x.bandwidthPriority < y.bandwidthPriority;
                case SortColumn::CompletedDate: return x.doneDate < y.doneDate;
            }
            return false;
        });

    grid()->setRowCount((int)visible_.size());
    grid()->refresh();
    updateCommandStates();
}

const Torrent* TorrentListWindow::selectedTorrent() const {
    int row = grid()->focusedRow();
    if (row < 0 || row >= (int)visible_.size()) return nullptr;
    return &visible_[row];
}

std::vector<const Torrent*> TorrentListWindow::targetTorrents() const {
    std::vector<const Torrent*> out;
    if (grid()->isInSelectionMode()) {
        for (int row : grid()->selectedRows()) {
            if (row >= 0 && row < (int)visible_.size()) out.push_back(&visible_[row]);
        }
        if (!out.empty()) return out;
        // In selection mode but nothing checked yet — fall through to
        // the focused row below, same as not being in selection mode
        // at all, rather than an action silently doing nothing.
    }
    if (const Torrent* t = selectedTorrent()) out.push_back(t);
    return out;
}

void TorrentListWindow::updateCommandStates() {
    const Torrent* t = selectedTorrent();
    if (!t) {
        // Nothing selected (e.g. empty list): no per-torrent action
        // makes sense.
        setCmd(this, cmStartTorrent, false);
        setCmd(this, cmStopTorrent, false);
        setCmd(this, cmRemoveTorrent, false);
        setCmd(this, cmDeleteTorrentWithData, false);
        setCmd(this, cmVerifyTorrent, false);
        setCmd(this, cmReannounceTorrent, false);
        setCmd(this, cmStartNowTorrent, false);
        setCmd(this, cmShowDetails, false);
        setCmd(this, cmShowFiles, false);
        return;
    }
    bool stopped = isStopped(*t);
    bool queued = isQueued(*t);
    bool active = !stopped;

    setCmd(this, cmStartTorrent, stopped);           // already running/queued: nothing to start
    setCmd(this, cmStopTorrent, active);              // already stopped: nothing to stop
    setCmd(this, cmRemoveTorrent, true);              // always possible
    setCmd(this, cmDeleteTorrentWithData, true);       // always possible
    setCmd(this, cmVerifyTorrent, true);               // Transmission allows this in any state
    setCmd(this, cmReannounceTorrent, active);         // only meaningful while talking to trackers
    setCmd(this, cmStartNowTorrent, stopped || queued); // only useful if not already transferring
    setCmd(this, cmShowDetails, true);                 // always possible
    setCmd(this, cmShowFiles, true);                   // always possible
}

void TorrentListWindow::showContextMenuFor(int /*row*/, TPoint screenPos) {
    // TMenuBox/TMenuPopup size themselves from their content and anchor
    // at bounds.a, expanding toward bounds.b — a small bounds.b here
    // (rather than a comfortably large one) would make it anchor
    // backwards from the click point instead of growing rightward/
    // downward from it (see getRect() in tvision's tmenubox.cpp).
    TRect r(screenPos.x, screenPos.y, screenPos.x + 40, screenPos.y + 10);
    TMenu* menu = new TMenu(
        *new TMenuItem(tr(Str::MenuStart), cmStartTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuStartNow), cmStartNowTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuStop), cmStopTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuVerify), cmVerifyTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuReannounce), cmReannounceTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuRemove), cmRemoveTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuDeleteWithData), cmDeleteTorrentWithData, kbNoKey) +
        *new TMenuItem(tr(Str::MenuShowDetails), cmShowDetails, kbNoKey) +
        *new TMenuItem(tr(Str::MenuShowFiles), cmShowFiles, kbNoKey)
    );
    auto* popup = new TMenuPopup(r, menu);
    // execView() (inherited from TProgram/TApplication) inserts the
    // popup, runs its own event loop until a choice is made or it's
    // dismissed, then removes it — the same mechanism tvision's own
    // pull-down submenus use internally (see newSubView()/execView() in
    // tmnuview.cpp) and the same one this app already uses for its own
    // modal dialogs (see App.cpp's execView(dlg) calls).
    ushort chosen = TProgram::application->execView(popup);
    TObject::destroy(popup);
    if (chosen != 0 && commandEnabled(chosen)) {
        // Re-emit exactly as a button or menu item would (see
        // TButton::press() in tbutton.cpp): this is the same command
        // value App::handleEvent already dispatches for the Torrent
        // menu and the status bar, so it reaches the same handling
        // without duplicating it here.
        TEvent e;
        e.what = evCommand;
        e.message.command = chosen;
        e.message.infoPtr = this;
        putEvent(e);
    }
}

void TorrentListWindow::showDetailsForSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    // Copied before the loop below: getTorrentDetails()/refresh() calls
    // partway through could otherwise invalidate the Torrent* pointers
    // targetTorrents() returned (they point into visible_).
    std::vector<int> ids;
    for (const Torrent* t : targets) ids.push_back(t->id);

    TDeskTop* deskTop = TProgram::deskTop;
    for (int id : ids) {
        // Look for an already-open details window for this same torrent
        // id before creating a new one — same deskTop->last/next
        // traversal already used for the "Window list" dialog (see
        // App.cpp). Order doesn't matter here either: we're searching
        // for a specific id, not relying on position.
        bool foundExisting = false;
        if (deskTop->last) {
            TView* p = deskTop->last;
            do {
                p = p->next;
                if (auto* existing = dynamic_cast<TorrentDetailsWindow*>(p)) {
                    if (existing->torrentId() == id) {
                        existing->select(); // bring the existing one to front instead
                        foundExisting = true;
                        break;
                    }
                }
            } while (p != deskTop->last);
        }
        if (foundExisting) continue;

        // The main list only carries listTorrents()'s lightweight fields
        // (see TransmissionClient.h) — the details window needs more
        // (location, privacy, magnet link, piece info, all-time transfer
        // totals, activity/elapsed-time fields), fetched here on demand
        // rather than on every periodic refresh.
        Torrent details = client_.getTorrentDetails(id);
        if (auto* win = createTorrentDetailsWindow(details, client_,
                                                    initialTrackerColumnWidths_,
                                                    initialTrackerColumnOrder_,
                                                    initialTrackerColumnVisible_))
            TProgram::application->insertWindow(win);
    }
    grid()->exitSelectionMode();
}

void TorrentListWindow::showFilesForSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    // Copied before the loop below for the same reason
    // showDetailsForSelected() does: names/ids point into visible_,
    // which a refresh() partway through the loop could invalidate.
    std::vector<std::pair<int, std::string>> idsAndNames;
    for (const Torrent* t : targets) idsAndNames.emplace_back(t->id, t->name);

    TDeskTop* deskTop = TProgram::deskTop;
    for (const auto& [id, name] : idsAndNames) {
        // Same "find an already-open one for this torrent id" check as
        // showDetailsForSelected() above, for the same reason: bring the
        // existing files window to front instead of opening a duplicate.
        bool foundExisting = false;
        if (deskTop->last) {
            TView* p = deskTop->last;
            do {
                p = p->next;
                if (auto* existing = dynamic_cast<TorrentFilesWindow*>(p)) {
                    if (existing->torrentId() == id) {
                        existing->select();
                        foundExisting = true;
                        break;
                    }
                }
            } while (p != deskTop->last);
        }
        if (foundExisting) continue;

        if (auto* win = createTorrentFilesWindow(id, name, client_))
            TProgram::application->insertWindow(win);
    }
    grid()->exitSelectionMode();
}

void TorrentListWindow::startSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.startTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::stopSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.stopTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::removeSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    std::string msg = (targets.size() == 1)
        ? formatMessage(tr(Str::ConfirmRemoveTorrent), targets[0]->name)
        : formatMessage(tr(Str::ConfirmRemoveTorrentsMulti), std::to_string(targets.size()));
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    for (const Torrent* t : targets) client_.removeTorrent(t->id, /*deleteLocalData=*/false);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::deleteWithDataSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    std::string msg = (targets.size() == 1)
        ? formatMessage(tr(Str::ConfirmDeleteTorrentWithData), targets[0]->name)
        : formatMessage(tr(Str::ConfirmDeleteTorrentsWithDataMulti), std::to_string(targets.size()));
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    for (const Torrent* t : targets) client_.removeTorrent(t->id, /*deleteLocalData=*/true);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::startNowSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.startTorrentNow(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::verifySelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.verifyTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::reannounceSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.reannounceTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

double TorrentListWindow::totalDownloadRate() const {
    double total = 0.0;
    for (const auto& t : visible_) total += t.rateDownload;
    return total;
}

double TorrentListWindow::totalUploadRate() const {
    double total = 0.0;
    for (const auto& t : visible_) total += t.rateUpload;
    return total;
}
