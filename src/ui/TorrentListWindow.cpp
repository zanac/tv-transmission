#include "TorrentListWindow.h"
#include "TorrentDetailsWindow.h"
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
                                      SortChangedCallback onSortChanged)
    : TWindowInit(&TWindow::initFrame), // virtual base: must be initialized here,
                                         // by the most-derived class — TGridWindow's
                                         // own initialization of it doesn't propagate
                                         // through another level of inheritance
      TGridWindow(bounds, tr(Str::WindowTitleTorrentList), /*fullScreen=*/true,
                  gvResizableColumns | gvReorderableColumns),
      client_(client), filter_(std::move(initialFilter)),
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

void TorrentListWindow::startColumnResize(int col) {
    grid()->startKeyboardResize(col);
}

void TorrentListWindow::startColumnReorder(int col) {
    grid()->startKeyboardReorder(col);
}

std::vector<int> TorrentListWindow::columnOrder() const {
    return grid()->columnOrder();
}

std::vector<bool> TorrentListWindow::columnVisibility() const {
    std::vector<bool> vis(7, true);
    for (int i = 0; i < 7; i++) vis[i] = grid()->isColumnVisible(i);
    return vis;
}

void TorrentListWindow::setColumnVisibility(const std::vector<bool>& visible) {
    for (int i = 0; i < 7; i++) {
        bool shown = (i < (int)visible.size()) ? visible[i] : true;
        grid()->setColumnVisible(i, shown);
    }
}

void TorrentListWindow::resetColumnLayout() {
    // setupColumns({}) re-adds all 7 columns fresh with their built-in
    // default widths (see its own body) — TGridView::clearColumns()/
    // addColumn() each reset the display order to identity as a side
    // effect (see TGridView.h's comment on why), so order comes back to
    // Name..Status left-to-right for free; visibility needs its own
    // pass since clearing/re-adding columns doesn't touch it.
    setupColumns({});
    for (int i = 0; i < 7; i++) grid()->setColumnVisible(i, true);
    applyFilterAndSort(); // row count/content are unaffected by any of
                          // this, but the columns were just torn down
                          // and rebuilt, so the grid needs telling again
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
        *new TMenuItem(tr(Str::MenuShowDetails), cmShowDetails, kbNoKey)
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
    const Torrent* t = selectedTorrent();
    if (!t) return;
    int id = t->id; // copy before any refresh() invalidates the pointer below

    // Look for an already-open details window for this same torrent id
    // before creating a new one — same deskTop->last/next traversal
    // already used for the "Window list" dialog (see App.cpp). Order
    // doesn't matter here either: we're searching for a specific id, not
    // relying on position.
    TDeskTop* deskTop = TProgram::deskTop;
    if (deskTop->last) {
        TView* p = deskTop->last;
        do {
            p = p->next;
            if (auto* existing = dynamic_cast<TorrentDetailsWindow*>(p)) {
                if (existing->torrentId() == id) {
                    existing->select(); // bring the existing one to front instead
                    return;
                }
            }
        } while (p != deskTop->last);
    }

    // The main list only carries listTorrents()'s lightweight fields
    // (see TransmissionClient.h) — the details window needs more
    // (location, privacy, magnet link, piece info, all-time transfer
    // totals, activity/elapsed-time fields), fetched here on demand
    // rather than on every periodic refresh.
    Torrent details = client_.getTorrentDetails(id);
    if (auto* win = createTorrentDetailsWindow(details, client_))
        TProgram::application->insertWindow(win);
}

void TorrentListWindow::startSelected() {
    if (const Torrent* t = selectedTorrent())
        client_.startTorrent(t->id);
    refresh();
}

void TorrentListWindow::stopSelected() {
    if (const Torrent* t = selectedTorrent())
        client_.stopTorrent(t->id);
    refresh();
}

void TorrentListWindow::removeSelected() {
    const Torrent* t = selectedTorrent();
    if (!t) return;
    std::string msg = formatMessage(tr(Str::ConfirmRemoveTorrent), t->name);
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    client_.removeTorrent(t->id, /*deleteLocalData=*/false);
    refresh();
}

void TorrentListWindow::deleteWithDataSelected() {
    const Torrent* t = selectedTorrent();
    if (!t) return;
    std::string msg = formatMessage(tr(Str::ConfirmDeleteTorrentWithData), t->name);
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    client_.removeTorrent(t->id, /*deleteLocalData=*/true);
    refresh();
}

void TorrentListWindow::startNowSelected() {
    if (const Torrent* t = selectedTorrent())
        client_.startTorrentNow(t->id);
    refresh();
}

void TorrentListWindow::verifySelected() {
    if (const Torrent* t = selectedTorrent())
        client_.verifyTorrent(t->id);
    refresh();
}

void TorrentListWindow::reannounceSelected() {
    if (const Torrent* t = selectedTorrent())
        client_.reannounceTorrent(t->id);
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
