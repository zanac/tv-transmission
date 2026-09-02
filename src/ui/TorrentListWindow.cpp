#include "TorrentListWindow.h"
#include "TorrentDetailsWindow.h"
#include "Strings.h"
#include "../TextUtil.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TDrawBuffer
#define Uses_TMenuItem
#define Uses_TMenu
#define Uses_TMenuPopup
#define Uses_MsgBox
#define Uses_TKeys
#include <tvision/tv.h>
#include "App.h"

namespace {

// Column widths shared between the data row (getText), the header
// (TorrentListHeader) and click detection (columnAt), so they stay
// aligned by construction rather than by eyeballing them.
constexpr int kNameW   = 30; // name column
constexpr int kDoneW   = 6;  // "100.0%" in full
constexpr int kSizeW   = 10; // formatSize() output (adaptive unit), right-aligned
constexpr int kDownW   = 14; // "  D:     0KB/s" in full (spaces included)
constexpr int kUpW     = 13; // " U:     0KB/s" in full (space included)
constexpr int kAddedW  = 17; // "YYYY-MM-DD HH:MM" (16 chars) + 1
constexpr int kStatusW = 23; // longest status string across both languages + 1
                              // ("In attesa di verifica"/"In attesa di download" = 22 chars)

// Right-aligns a (plain ASCII) string to exactly `width` columns,
// padding with leading spaces. Unlike padOrTruncateUtf8 (used for the
// name column, left-aligned), this never truncates: a still-too-long
// string is left as-is rather than having digits silently cut off,
// which would turn a large-but-correct number into a smaller, wrong
// one. All values placed through this (formatSize()'s output,
// timestamps) are kept short by construction, so overflow here would
// only happen at values far beyond anything a real torrent produces.
std::string rightAlign(const std::string& s, size_t width) {
    if (s.size() >= width) return s;
    return std::string(width - s.size(), ' ') + s;
}

// [start,end) range of terminal columns occupied by each column of the
// header/row, in the same order as SortColumn. Built once from the
// kNameW/etc. constants above, so clicking and drawing always use the
// same math.
struct ColumnRange { int start, end; SortColumn column; };

std::vector<ColumnRange> columnRanges() {
    std::vector<ColumnRange> ranges;
    int pos = 0;
    ranges.push_back({pos, pos + kNameW, SortColumn::Name}); pos += kNameW + 1; // +1 separator space
    ranges.push_back({pos, pos + kDoneW, SortColumn::Done}); pos += kDoneW + 1; // +1 separator space
    ranges.push_back({pos, pos + kSizeW, SortColumn::Size}); pos += kSizeW;     // no space before Down
    ranges.push_back({pos, pos + kDownW, SortColumn::Down}); pos += kDownW;     // no space before Up
    ranges.push_back({pos, pos + kUpW,   SortColumn::Up});   pos += kUpW + 1;   // +1 separator space
    ranges.push_back({pos, pos + kAddedW, SortColumn::Added}); pos += kAddedW + 1; // +1 separator space
    ranges.push_back({pos, pos + kStatusW, SortColumn::Status});
    return ranges;
}

// Column clicked given a local x coordinate within the header, or -1 if
// the click landed on a separator space.
int columnAt(int x) {
    for (const auto& r : columnRanges())
        if (x >= r.start && x < r.end) return static_cast<int>(r.column);
    return -1;
}

// Base label for each column, indexed by SortColumn.
const char* baseLabel(SortColumn col) {
    switch (col) {
        case SortColumn::Name:   return tr(Str::HeaderName);
        case SortColumn::Done:   return tr(Str::HeaderDone);
        case SortColumn::Size:   return tr(Str::HeaderSize);
        case SortColumn::Down:   return tr(Str::HeaderDownload);
        case SortColumn::Up:     return tr(Str::HeaderUpload);
        case SortColumn::Added:  return tr(Str::HeaderAdded);
        case SortColumn::Status: return tr(Str::HeaderStatus);
    }
    return "";
}

// Header row with the exact same widths as getText(), plus a ^/v
// indicator on the column currently used for sorting.
// Note: unlike windows/dialogs that get rebuilt every time they're
// shown, this header does not update itself if the language changes at
// runtime — same limitation (and same reason) as the menu bar/status
// bar, see App.cpp/main.cpp.
std::string buildHeaderText(SortColumn sortColumn, bool ascending) {
    auto label = [&](SortColumn col) {
        std::string s = baseLabel(col);
        if (col == sortColumn) s += ascending ? " ^" : " v";
        return s;
    };
    char buf[192];
    std::snprintf(buf, sizeof(buf), "%-*s %*s %*s%*s%*s %*s %*s",
        kNameW, label(SortColumn::Name).c_str(),
        kDoneW, label(SortColumn::Done).c_str(),
        kSizeW, label(SortColumn::Size).c_str(),
        kDownW, label(SortColumn::Down).c_str(),
        kUpW,   label(SortColumn::Up).c_str(),
        kAddedW, label(SortColumn::Added).c_str(),
        kStatusW, label(SortColumn::Status).c_str());
    return buf;
}

} // namespace

// Minimal view that draws the header and handles clicks for sorting.
// Holds a pointer to the TorrentListViewer to know what to draw (current
// column/direction) and to apply the new sort on click.
class TorrentListHeader : public TView {
public:
    TorrentListHeader(const TRect& r, TorrentListViewer* listViewer)
        : TView(r), listViewer_(listViewer) {
        growMode = gfGrowHiX; // follows the window's width
    }

    void draw() override {
        TDrawBuffer b;
        // Yellow on blue: distinct from the rows (white on blue) while
        // staying in the same blue tones requested for the list.
        TColorAttr color(0x1E);
        std::string text = buildHeaderText(listViewer_->sortColumn(),
                                            listViewer_->sortAscending());
        b.moveChar(0, ' ', color, size.x);
        b.moveStr(0, text.c_str(), color);
        writeLine(0, 0, size.x, 1, b);
    }

    void handleEvent(TEvent& event) override {
        TView::handleEvent(event);
        if (event.what == evMouseDown) {
            TPoint p = makeLocal(event.mouse.where);
            int col = columnAt(p.x);
            if (col >= 0) {
                listViewer_->toggleSort(static_cast<SortColumn>(col));
                drawView();
            }
            clearEvent(event);
        }
    }

private:
    TorrentListViewer* listViewer_;
};

TorrentListViewer::TorrentListViewer(const TRect& r, TScrollBar* vScrollBar,
                                      SortColumn initialSort, bool initialAscending,
                                      SortChangedCallback onSortChanged)
    : TListViewer(r, 1, nullptr, vScrollBar),
      sortColumn_(initialSort), sortAscending_(initialAscending),
      onSortChanged_(std::move(onSortChanged)) {
    setRange(0);
}

void TorrentListViewer::setTorrents(std::vector<Torrent> torrents) {
    torrents_ = std::move(torrents);
    applySort(); // every refresh starts from unsorted server data: the
                 // user's chosen criterion is re-applied here
    setRange((short)torrents_.size());
    if (focused >= range && range > 0) focusItem(range - 1);
    updateCommandStates(); // the still-focused torrent's state may have
                           // changed even when its index didn't
    drawView();
}

const Torrent* TorrentListViewer::selectedTorrent() const {
    if (focused < 0 || focused >= (int)torrents_.size()) return nullptr;
    return &torrents_[focused];
}

double TorrentListViewer::totalDownloadRate() const {
    double total = 0.0;
    for (const auto& t : torrents_) total += t.rateDownload;
    return total;
}

double TorrentListViewer::totalUploadRate() const {
    double total = 0.0;
    for (const auto& t : torrents_) total += t.rateUpload;
    return total;
}

void TorrentListViewer::toggleSort(SortColumn column) {
    if (column == sortColumn_) sortAscending_ = !sortAscending_;
    else { sortColumn_ = column; sortAscending_ = true; }
    applySort();
    drawView();
    if (onSortChanged_) onSortChanged_(sortColumn_, sortAscending_);
}

void TorrentListViewer::applySort() {
    // Always compare "ascending" but with the arguments swapped for
    // descending order, instead of negating the result: negating `less`
    // to get `greater` breaks the strict-weak-ordering std::sort
    // requires when two elements are equal (a<b false AND b<a false, but
    // !less(a,b) would still be "true" both ways).
    std::sort(torrents_.begin(), torrents_.end(),
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
}

void TorrentListViewer::getText(char* dest, short item, short maxLen) {
    if (item < 0 || item >= (int)torrents_.size()) {
        dest[0] = '\0';
        return;
    }
    const Torrent& t = torrents_[item];
    std::string name = padOrTruncateUtf8(t.name, kNameW);
    std::string sizeStr = rightAlign(formatSize(t.sizeBytes), kSizeW);
    std::string addedStr = rightAlign(formatUnixTimestamp(t.addedDate), kAddedW);
    std::string statusStr = padOrTruncateUtf8(trTorrentStatus(t.status), kStatusW);
    std::snprintf(dest, maxLen,
        "%s %5.1f%% %s  D:%6.0fKB/s U:%6.0fKB/s %s %s",
        name.c_str(), t.percentDone * 100.0, sizeStr.c_str(),
        t.rateDownload / 1024.0, t.rateUpload / 1024.0,
        addedStr.c_str(), statusStr.c_str());
}

TColorAttr TorrentListViewer::mapColor(uchar index) {
    // mapColor() (unlike getColor()/getPalette(), which alone wouldn't
    // be enough here — see the comment in TorrentListWindow.h) is
    // virtual and gets called on the real `this` even when the caller is
    // TListViewer's inherited draw() — so this override is reached
    // correctly. It bypasses the owner's palette chain entirely: fixed
    // colors independent of the app's theme.
    switch (index) {
        case 1: case 2: return TColorAttr(0x1F); // normal rows: white on blue
        case 3: case 4: return TColorAttr(0xF0); // current row: black on white
        default: return TListViewer::mapColor(index);
    }
}

namespace {

// tr_torrent_activity values (Transmission RPC): 0=stopped,
// 1=check-wait, 2=checking, 3=download-wait, 4=downloading,
// 5=seed-wait, 6=seeding. "Queued" here means 1/3/5: already started
// (waiting for its turn), as opposed to genuinely stopped (0).
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

} // namespace

void TorrentListViewer::updateCommandStates() {
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

void TorrentListViewer::focusItem(short item) {
    TListViewer::focusItem(item);
    updateCommandStates();
}

void TorrentListViewer::handleEvent(TEvent& event) {
    TListViewer::handleEvent(event);
    if (event.what == evMouseDown && (event.mouse.buttons & mbRightButton) != 0) {
        TPoint where = event.mouse.where; // already in global/screen coordinates
        TPoint local = makeLocal(where);
        // Same formula TListViewer::handleEvent() itself uses for
        // left-click selection (see tlstview.cpp): with numCols == 1
        // (our case) each item is exactly one row, so the clicked
        // index is simply topItem + the local y offset.
        short row = topItem + local.y;
        if (row >= 0 && row < range) {
            focusItemNum(row); // also runs updateCommandStates() via the override above
            showContextMenu(where);
        }
        clearEvent(event);
    }
}

void TorrentListViewer::showContextMenu(TPoint where) {
    // TMenuBox/TMenuPopup size themselves from their content and anchor
    // at bounds.a, expanding toward bounds.b — a small bounds.b here
    // (rather than a comfortably large one) would make it anchor
    // backwards from the click point instead of growing rightward/
    // downward from it (see getRect() in tvision's tmenubox.cpp).
    TRect r(where.x, where.y, where.x + 40, where.y + 10);
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

TorrentListWindow::TorrentListWindow(const TRect& bounds, TransmissionClient& client,
                                      SortColumn initialSort, bool initialAscending,
                                      SortChangedCallback onSortChanged)
    : TWindowInit(&TWindow::initFrame),
      TWindow(bounds, tr(Str::WindowTitleTorrentList), wnNoNumber),
      client_(client) {
    options |= ofTileable;

    // This is the main window, kept pointed to by App::listWindow_,
    // always full-screen: no moving, resizing, zooming or closing. The
    // reason wfClose has to go too is the historical one below:
    // TWindow::close() would destroy the object (destroy(this)), leaving
    // App::listWindow_ a dangling pointer on the next idle() tick.
    flags = 0;

    TRect r = getExtent();
    r.grow(-1, -1);

    // Column header row, right below the window border; everything else
    // (scrollbar + list) moves down one row to make room for it.
    TRect headerRect(r.a.x, r.a.y, r.b.x, r.a.y + 1);
    r.a.y += 1;

    // FIX: the scrollbar used to occupy the last column, but the list
    // was created as wide as the whole `r` (including that column), and
    // being drawn ON TOP of the scrollbar (inserted after it) it covered
    // it completely — hence "the scrollbars are missing" even though
    // they were there. The list is now `r` minus the scrollbar's column.
    TRect scrollRect(r.b.x - 1, r.a.y, r.b.x, r.b.y);
    TRect listRect(r.a.x, r.a.y, r.b.x - 1, r.b.y);

    TScrollBar* vScroll = new TScrollBar(scrollRect);
    insert(vScroll);

    listViewer_ = new TorrentListViewer(listRect, vScroll, initialSort, initialAscending,
                                         std::move(onSortChanged));
    insert(listViewer_);

    insert(new TorrentListHeader(headerRect, listViewer_));

    refresh();
}

void TorrentListWindow::retranslate() {
    // title is allocated with newStr() by TWindow's constructor (see
    // twindow.cpp) and freed with delete[] in its destructor — the same
    // pattern used for TStatusItem::text in BandwidthStatusLine.
    delete[] (char*)title;
    title = newStr(tr(Str::WindowTitleTorrentList));
    drawView();
}

void TorrentListWindow::handleEvent(TEvent& event) {
    TWindow::handleEvent(event);
    // TListViewer::selectItem() sends this broadcast to its own owner
    // (this window) when an item is selected via double-click (see
    // tlstview.cpp: meDoubleClick -> selectItem()).
    if (event.what == evBroadcast &&
        event.message.command == cmListItemSelected &&
        event.message.infoPtr == listViewer_) {
        showDetailsForSelected();
        clearEvent(event);
    }
}

void TorrentListWindow::showDetailsForSelected() {
    const Torrent* t = listViewer_->selectedTorrent();
    if (!t) return;

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
                if (existing->torrentId() == t->id) {
                    existing->select(); // bring the existing one to front instead
                    return;
                }
            }
        } while (p != deskTop->last);
    }

    if (auto* win = createTorrentDetailsWindow(*t, client_))
        TProgram::application->insertWindow(win);
}

void TorrentListWindow::refresh() {
    if (!listViewer_) return;
    listViewer_->setTorrents(client_.listTorrents());
}

void TorrentListWindow::startSelected() {
    if (const Torrent* t = listViewer_->selectedTorrent())
        client_.startTorrent(t->id);
    refresh();
}

void TorrentListWindow::stopSelected() {
    if (const Torrent* t = listViewer_->selectedTorrent())
        client_.stopTorrent(t->id);
    refresh();
}

void TorrentListWindow::removeSelected() {
    const Torrent* t = listViewer_->selectedTorrent();
    if (!t) return;
    std::string msg = formatMessage(tr(Str::ConfirmRemoveTorrent), t->name);
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    client_.removeTorrent(t->id, /*deleteLocalData=*/false);
    refresh();
}

void TorrentListWindow::deleteWithDataSelected() {
    const Torrent* t = listViewer_->selectedTorrent();
    if (!t) return;
    std::string msg = formatMessage(tr(Str::ConfirmDeleteTorrentWithData), t->name);
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    client_.removeTorrent(t->id, /*deleteLocalData=*/true);
    refresh();
}

void TorrentListWindow::startNowSelected() {
    if (const Torrent* t = listViewer_->selectedTorrent())
        client_.startTorrentNow(t->id);
    refresh();
}

void TorrentListWindow::verifySelected() {
    if (const Torrent* t = listViewer_->selectedTorrent())
        client_.verifyTorrent(t->id);
    refresh();
}

void TorrentListWindow::reannounceSelected() {
    if (const Torrent* t = listViewer_->selectedTorrent())
        client_.reannounceTorrent(t->id);
    refresh();
}

double TorrentListWindow::totalDownloadRate() const {
    return listViewer_ ? listViewer_->totalDownloadRate() : 0.0;
}

double TorrentListWindow::totalUploadRate() const {
    return listViewer_ ? listViewer_->totalUploadRate() : 0.0;
}
