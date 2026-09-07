#include "TGridView.h"

#define Uses_TDrawBuffer
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TEvent
#define Uses_TKeys
#include <tvision/tv.h>

#include <algorithm>
#include <cstring>

namespace {

// --- Minimal, self-contained UTF-8 helpers -------------------------------
// Deliberately reimplemented here rather than shared with the rest of the
// project: this folder has no dependency on anything outside tvision + the
// standard library (see the comment at the top of TGridView.h).

std::vector<size_t> codepointStarts(const std::string& s) {
    std::vector<size_t> starts;
    for (size_t i = 0; i < s.size(); ) {
        starts.push_back(i);
        unsigned char c = (unsigned char)s[i];
        size_t len = (c < 0x80) ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
        i += len;
    }
    return starts;
}

int codepointCount(const std::string& s) { return (int)codepointStarts(s).size(); }

std::string truncateUtf8(const std::string& s, int maxWidth) {
    auto starts = codepointStarts(s);
    if ((int)starts.size() <= maxWidth) return s;
    if (maxWidth <= 0) return "";
    return s.substr(0, starts[maxWidth]);
}

// Pads/truncates to exactly `width` columns, aligned as requested.
std::string fitToWidth(const std::string& s, int width, TGridColumn::Align align) {
    if (width <= 0) return "";
    std::string t = truncateUtf8(s, width);
    int pad = width - codepointCount(t);
    if (pad <= 0) return t;
    return align == TGridColumn::Align::Right
        ? std::string(pad, ' ') + t
        : t + std::string(pad, ' ');
}

constexpr int kSeparatorWidth = 1; // one character between adjacent columns

} // namespace

// ===========================================================================
// TGridHeaderView — draws column headers; when gvResizableColumns is set,
// dragging the single-character separator between two headers resizes the
// column to its left (see README.md for why "left column only", not a
// proportional split, was chosen). When gvReorderableColumns is set,
// double-clicking a column's name enters TGridView::runReorderLoop().
//
// Every method here works in VISUAL positions — indices into
// owner_->visibleDisplayOrder() specifically, i.e. positions among only
// the columns that are currently shown (see TGridColumn::visible) — never
// raw indices into owner_->displayOrder_, which also includes hidden
// columns. Translating to the LOGICAL index (the column's stable
// identity, what the owner's own callbacks expect — see the big comment
// on TGridView::column() in TGridView.h) happens at the point of actually
// calling into the owner. Kept local to this class rather than pushed
// into TGridView itself because hit-testing (which visual position a
// click/drag lands on) is inherently about what's currently drawn, which
// only this view computes.
// ===========================================================================
class TGridHeaderView : public TView {
public:
    TGridHeaderView(const TRect& r, TGridView* owner)
        : TView(r), owner_(owner) {
        growMode = gfGrowHiX;
        eventMask |= evMouseDown;
    }

    // Same single palette slot TStaticText uses (see cpStaticText in
    // tvision's tstatict.cpp: "\x06") — a header showing text is no
    // different a visual role than a label, so by default it should
    // look identical rather than inventing a new hardcoded color. An
    // app embedding TGridView is free to recolor by overriding this
    // view's palette resolution the normal tvision way (its own
    // getPalette() chain), since this is just a TView like any other.
    TPalette& getPalette() const override {
        static TPalette palette("\x06", 1);
        return palette;
    }

    void draw() override {
        TDrawBuffer b;
        TColorAttr color = getColor(1);
        b.moveChar(0, ' ', color, size.x);
        int x = 0;
        auto vis = owner_->visibleDisplayOrder();
        int n = (int)vis.size();
        for (int visualPos = 0; visualPos < n; visualPos++) {
            int logicalCol = vis[visualPos];
            const TGridColumn& col = owner_->column(logicalCol);
            // The sort indicator is appended here, at draw time, rather
            // than stored in TGridColumn::header — so changing a
            // column's label (e.g. a language switch) never has to
            // remember to strip/reapply it first. Same reasoning for the
            // "<"/">" reorder markers just below.
            std::string text = col.header;
            if (logicalCol == owner_->sortColumn_) text += owner_->sortAscending_ ? " ^" : " v";
            if (visualPos == owner_->reorderVisualPos_) {
                // No marker on whichever side doesn't apply — there's
                // nowhere further to move at either edge of what's
                // currently visible.
                std::string left = (visualPos > 0) ? "<" : "";
                std::string right = (visualPos < n - 1) ? ">" : "";
                text = left + text + right;
            }
            text = fitToWidth(text, col.width, col.align);
            x += b.moveStr(x, text.c_str(), color);
            if (visualPos < n - 1) {
                // The separator doubles as the resize handle's visual
                // cue when resizing is enabled — "│" makes the grabbable
                // boundary visible instead of it being an invisible gap
                // the user has to guess at.
                const char* sep = (owner_->options_ & gvResizableColumns) ? "\xE2\x94\x82" /* │ */ : " ";
                x += b.moveStr(x, sep, color);
            }
        }
        writeLine(0, 0, size.x, 1, b);
    }

    void handleEvent(TEvent& event) override {
        TView::handleEvent(event);
        if (event.what != evMouseDown) return;

        auto vis = owner_->visibleDisplayOrder();
        TPoint local = makeLocal(event.mouse.where);
        int visualPos = columnAtX(local.x, vis);

        // Checked before resize/sort: a double-click on a column's name
        // (not its separator) always means "start reordering", taking
        // priority over the single-click sort-toggle that would
        // otherwise also fire for the same click.
        if ((owner_->options_ & gvReorderableColumns) &&
            (event.mouse.eventFlags & meDoubleClick) && visualPos >= 0) {
            int logicalCol = vis[visualPos];
            if (owner_->column(logicalCol).movable) {
                owner_->runReorderLoop(visualPos);
                clearEvent(event);
                return;
            }
        }

        if ((owner_->options_ & gvResizableColumns) && isOnSeparator(local.x, visualPos, vis)) {
            dragResize(vis[visualPos], event);
            clearEvent(event);
            return;
        }
        if (visualPos >= 0) {
            int logicalCol = vis[visualPos];
            if (owner_->column(logicalCol).sortable) {
                bool ascending = (logicalCol == owner_->sortColumn_) ? !owner_->sortAscending_ : true;
                owner_->setSortIndicator(logicalCol, ascending);
                if (owner_->onSortChanged_) owner_->onSortChanged_(logicalCol, ascending);
                clearEvent(event);
            }
        }
    }

private:
    // VISUAL position (index into `vis`, not a logical index) whose span
    // (including its trailing separator) contains local x, or -1 if past
    // the last visible column.
    int columnAtX(int x, const std::vector<int>& vis) const {
        int pos = 0;
        int n = (int)vis.size();
        for (int visualPos = 0; visualPos < n; visualPos++) {
            int w = owner_->column(vis[visualPos]).width + (visualPos < n - 1 ? kSeparatorWidth : 0);
            if (x >= pos && x < pos + w) return visualPos;
            pos += w;
        }
        return -1;
    }

    // True if x lands exactly on the separator column right after visual
    // position `visualPos` (i.e. the resize handle between it and the
    // next visible one).
    bool isOnSeparator(int x, int visualPos, const std::vector<int>& vis) const {
        if (visualPos < 0 || visualPos >= (int)vis.size() - 1) return false;
        int pos = 0;
        for (int i = 0; i < visualPos; i++) pos += owner_->column(vis[i]).width + kSeparatorWidth;
        pos += owner_->column(vis[visualPos]).width;
        return x == pos;
    }

    void dragResize(int logicalCol, TEvent& event) {
        int startX = event.mouse.where.x;
        int startWidth = owner_->column(logicalCol).width;
        while (mouseEvent(event, evMouseMove)) {
            int delta = event.mouse.where.x - startX;
            owner_->setColumnWidth(logicalCol, startWidth + delta);
            owner_->relayout();
        }
    }

    TGridView* owner_;
};

// ===========================================================================
// TGridRowsView — the actual scrolling rows, rendered entirely from
// TGridView's callbacks (see TGridView.h's "Data source" section for why
// there's no row-data storage here at all). Iterates columns via
// owner_->visibleDisplayOrder() for the same reason the header does — see
// its own doc comment above.
// ===========================================================================
class TGridRowsView : public TListViewer {
public:
    TGridRowsView(const TRect& r, TScrollBar* vScroll, TGridView* owner)
        : TListViewer(r, 1, nullptr, vScroll), owner_(owner) {
        setRange(0);
    }

    void getText(char* dest, short item, short maxLen) override {
        std::string line = buildRow(item);
        std::snprintf(dest, maxLen, "%s", line.c_str());
    }

    // Notifies TGridView::onRowFocus_ on every focus change — arrow-key
    // navigation, a plain click, or a caller's own focusRow() all funnel
    // through this one override (see focusItem()'s doc comment on
    // TListViewer itself: every other focus-changing method calls this).
    void focusItem(short item) override {
        TListViewer::focusItem(item);
        if (owner_->onRowFocus_) owner_->onRowFocus_(item);
    }

    void draw() override {
        TDrawBuffer b;
        auto vis = owner_->visibleDisplayOrder();
        for (short i = 0; i < size.y; i++) {
            short item = topItem + i;
            bool isFocused = (item == focused);
            // Always goes through the callback (with the correct
            // `focused` flag) when one is set, rather than only for
            // non-focused rows: a caller may want a specific focused-row
            // look (e.g. this project's black-on-white, distinct from
            // tvision's own default "selected" palette color) just as
            // much as a per-status color for the rest.
            TColorAttr rowColor = owner_->rowColor_
                ? owner_->rowColor_(item, isFocused)
                : TColorAttr(isFocused ? getColor(2) : getColor(1));
            b.moveChar(0, ' ', rowColor, size.x);
            if (item >= 0 && item < owner_->rowCount_) {
                int x = 0;
                int n = (int)vis.size();
                for (int visualPos = 0; visualPos < n; visualPos++) {
                    int logicalCol = vis[visualPos];
                    const TGridColumn& col = owner_->column(logicalCol);
                    std::string cellStr = owner_->cellText_ ? owner_->cellText_(item, logicalCol) : "";
                    std::string fitted = fitToWidth(cellStr, col.width, col.align);
                    TColorAttr cellColor = rowColor;
                    // Applied regardless of focus state — bold is a
                    // property of the cell (e.g. "this is the name
                    // column"), not something that should silently stop
                    // applying just because the row happens to be
                    // selected right now.
                    if (owner_->cellBold_ && owner_->cellBold_(item, logicalCol)) {
                        cellColor = TColorAttr(cellColor.getForeground(), cellColor.getBackground(),
                                                cellColor.getStyle() | slBold);
                    }
                    x += b.moveStr(x, fitted.c_str(), cellColor);
                    if (visualPos < n - 1) x += b.moveStr(x, " ", rowColor);
                }
            }
            writeLine(0, i, size.x, 1, b);
        }
    }

    void handleEvent(TEvent& event) override {
        TListViewer::handleEvent(event);
        if (event.what == evMouseDown && (event.mouse.buttons & mbRightButton) != 0 &&
            owner_->onRowContext_) {
            TPoint local = makeLocal(event.mouse.where);
            short row = topItem + local.y;
            if (row >= 0 && row < range) {
                focusItemNum(row);
                owner_->onRowContext_(row, event.mouse.where);
            }
            clearEvent(event);
        }
    }

private:
    // Used by getText() (plain, used by TListViewer internals such as
    // any future type-ahead search) — draw() builds the same content
    // per-segment instead, for per-cell coloring, but the column
    // iteration itself (via visibleDisplayOrder()) matches so the two
    // never drift apart on layout.
    std::string buildRow(int item) {
        if (item < 0 || item >= owner_->rowCount_) return "";
        std::string out;
        auto vis = owner_->visibleDisplayOrder();
        int n = (int)vis.size();
        for (int visualPos = 0; visualPos < n; visualPos++) {
            int logicalCol = vis[visualPos];
            const TGridColumn& col = owner_->column(logicalCol);
            std::string cellStr = owner_->cellText_ ? owner_->cellText_(item, logicalCol) : "";
            out += fitToWidth(cellStr, col.width, col.align);
            if (visualPos < n - 1) out += " ";
        }
        return out;
    }

    TGridView* owner_;
};

// ===========================================================================
// TGridView
// ===========================================================================

TGridView::TGridView(const TRect& bounds, ushort options)
    : TGroup(bounds), options_(options) {
    TRect r = getExtent();

    TRect headerRect(r.a.x, r.a.y, r.b.x, r.a.y + 1);
    TRect scrollRect(r.b.x - 1, r.a.y + 1, r.b.x, r.b.y);
    TRect rowsRect(r.a.x, r.a.y + 1, r.b.x - 1, r.b.y);

    scrollBar_ = new TScrollBar(scrollRect);
    scrollBar_->growMode = gfGrowLoX | gfGrowHiX | gfGrowHiY;
    insert(scrollBar_);

    rows_ = new TGridRowsView(rowsRect, scrollBar_, this);
    rows_->growMode = gfGrowHiX | gfGrowHiY;
    insert(rows_);

    header_ = new TGridHeaderView(headerRect, this);
    insert(header_);
}

int TGridView::addColumn(const TGridColumn& col) {
    columns_.push_back(col);
    resetColumnOrder();
    relayout();
    return (int)columns_.size() - 1;
}

void TGridView::insertColumn(int index, const TGridColumn& col) {
    index = std::clamp(index, 0, (int)columns_.size());
    columns_.insert(columns_.begin() + index, col);
    resetColumnOrder();
    relayout();
}

void TGridView::removeColumn(int index) {
    if (index < 0 || index >= (int)columns_.size()) return;
    columns_.erase(columns_.begin() + index);
    resetColumnOrder();
    relayout();
}

void TGridView::clearColumns() {
    columns_.clear();
    resetColumnOrder();
    relayout();
}

void TGridView::setColumnWidth(int index, int width) {
    if (index < 0 || index >= (int)columns_.size()) return;
    columns_[index].width = std::max(width, columns_[index].minWidth);
}

void TGridView::setColumnVisible(int index, bool visible) {
    if (index < 0 || index >= (int)columns_.size()) return;
    columns_[index].visible = visible;
    relayout();
}

void TGridView::setRowCount(int count) {
    rowCount_ = std::max(count, 0);
}

void TGridView::setCellTextCallback(CellTextFn fn) { cellText_ = std::move(fn); }
void TGridView::setRowColorCallback(RowColorFn fn) { rowColor_ = std::move(fn); }
void TGridView::setCellBoldCallback(CellBoldFn fn) { cellBold_ = std::move(fn); }
void TGridView::setRowActivateCallback(RowActivateFn fn) { onRowActivate_ = std::move(fn); }
void TGridView::setRowContextCallback(RowContextFn fn) { onRowContext_ = std::move(fn); }
void TGridView::setRowFocusCallback(RowFocusFn fn) { onRowFocus_ = std::move(fn); }
void TGridView::setSortChangedCallback(SortChangedFn fn) { onSortChanged_ = std::move(fn); }
void TGridView::setColumnOrderChangedCallback(ColumnOrderChangedFn fn) { onColumnOrderChanged_ = std::move(fn); }

void TGridView::setSortIndicator(int col, bool ascending) {
    sortColumn_ = col;
    sortAscending_ = ascending;
    if (header_) header_->drawView();
}

void TGridView::startKeyboardResize(int col) {
    if (col < 0 || col >= (int)columns_.size()) return;
    if (!columns_[col].resizable) return;

    int originalWidth = columns_[col].width;
    TEvent event;
    for (;;) {
        // Same "pump events in a loop" primitive mouseEvent() itself
        // is built on (see tview.cpp) — any TView can call getEvent()
        // to synchronously pull the next event, which is what makes
        // this callable from outside the header's own event handling
        // (e.g. a menu item), not just from a mouse-down already being
        // processed there.
        getEvent(event);
        if (event.what != evKeyDown) continue;
        switch (event.keyDown.keyCode) {
            case kbLeft:
                setColumnWidth(col, columns_[col].width - 1);
                relayout();
                break;
            case kbRight:
                setColumnWidth(col, columns_[col].width + 1);
                relayout();
                break;
            case kbEnter:
                return; // confirmed at the current width
            case kbEsc:
                setColumnWidth(col, originalWidth);
                relayout();
                return; // cancelled: reverted to the width it had on entry
            default:
                break; // any other key: ignored, keep waiting
        }
    }
}

std::vector<int> TGridView::visibleDisplayOrder() const {
    std::vector<int> vis;
    vis.reserve(displayOrder_.size());
    for (int logicalCol : displayOrder_)
        if (columns_[logicalCol].visible) vis.push_back(logicalCol);
    return vis;
}

int TGridView::visualPositionOf(int logicalCol) const {
    for (int i = 0; i < (int)displayOrder_.size(); i++)
        if (displayOrder_[i] == logicalCol) return i;
    return -1;
}

int TGridView::visiblePositionOf(int logicalCol) const {
    auto vis = visibleDisplayOrder();
    for (int i = 0; i < (int)vis.size(); i++)
        if (vis[i] == logicalCol) return i;
    return -1;
}

void TGridView::resetColumnOrder() {
    displayOrder_.resize(columns_.size());
    for (int i = 0; i < (int)displayOrder_.size(); i++) displayOrder_[i] = i;
}

void TGridView::setColumnOrder(const std::vector<int>& order) {
    // Must be exactly a permutation of [0, columnCount()) — anything
    // else (wrong size, an out-of-range or duplicate index) would leave
    // some column undrawable or drawn twice, so it's rejected wholesale
    // rather than applied partially.
    if (order.size() != columns_.size()) return;
    std::vector<bool> seen(columns_.size(), false);
    for (int v : order) {
        if (v < 0 || v >= (int)columns_.size() || seen[v]) return;
        seen[v] = true;
    }
    displayOrder_ = order;
    relayout();
}

void TGridView::startKeyboardReorder(int col) {
    if (col < 0 || col >= (int)columns_.size()) return;
    if (!columns_[col].movable || !columns_[col].visible) return;
    int visPos = visiblePositionOf(col);
    if (visPos < 0) return;
    runReorderLoop(visPos);
}

void TGridView::runReorderLoop(int startVisualPos) {
    reorderVisualPos_ = startVisualPos;
    std::vector<int> originalOrder = displayOrder_;
    header_->drawView();

    // Swaps the visible-list neighbors at positions `a` and `a+1` — found
    // via their TRUE (possibly non-adjacent, if hidden columns sit
    // between them) positions in displayOrder_, since that's the array
    // actually being reordered; visibleDisplayOrder() is just a filtered
    // view over it, recomputed fresh here because the previous swap may
    // have changed it.
    auto swapVisibleNeighbors = [this](int a, int b) {
        auto vis = visibleDisplayOrder();
        int rawA = visualPositionOf(vis[a]);
        int rawB = visualPositionOf(vis[b]);
        std::swap(displayOrder_[rawA], displayOrder_[rawB]);
    };

    TEvent event;
    for (;;) {
        // Same getEvent()-loop primitive as startKeyboardResize() — see
        // its own comment for why this works both from a menu command
        // and (via the header's double-click handling) from within an
        // event already being processed.
        getEvent(event);
        if (event.what == evKeyDown) {
            switch (event.keyDown.keyCode) {
                case kbLeft:
                    if (reorderVisualPos_ > 0) {
                        swapVisibleNeighbors(reorderVisualPos_, reorderVisualPos_ - 1);
                        reorderVisualPos_--;
                        relayout();
                    }
                    continue;
                case kbRight:
                    if (reorderVisualPos_ < (int)visibleDisplayOrder().size() - 1) {
                        swapVisibleNeighbors(reorderVisualPos_, reorderVisualPos_ + 1);
                        reorderVisualPos_++;
                        relayout();
                    }
                    continue;
                case kbEnter:
                    reorderVisualPos_ = -1;
                    header_->drawView();
                    if (onColumnOrderChanged_) onColumnOrderChanged_(displayOrder_);
                    return;
                case kbEsc:
                    displayOrder_ = originalOrder;
                    reorderVisualPos_ = -1;
                    relayout();
                    return; // cancelled: nothing actually changed, no callback
                default:
                    continue;
            }
        } else if (event.what == evMouseDown) {
            TPoint local = header_->makeLocal(event.mouse.where);
            int hit = reorderArrowHitTest(local.x);
            auto vis = visibleDisplayOrder();
            if (hit == -1 && reorderVisualPos_ > 0) {
                swapVisibleNeighbors(reorderVisualPos_, reorderVisualPos_ - 1);
                reorderVisualPos_--;
                relayout();
                continue;
            }
            if (hit == 1 && reorderVisualPos_ < (int)vis.size() - 1) {
                swapVisibleNeighbors(reorderVisualPos_, reorderVisualPos_ + 1);
                reorderVisualPos_++;
                relayout();
                continue;
            }
            // Anything else — a click inside the highlighted column but
            // not on a marker, or outside it entirely — confirms, same
            // as Enter: it's a deliberate "leave it here" either way,
            // and a click outside the column would otherwise be
            // silently swallowed instead of acting on whatever it was
            // actually meant for once this mode ends.
            reorderVisualPos_ = -1;
            header_->drawView();
            if (onColumnOrderChanged_) onColumnOrderChanged_(displayOrder_);
            return;
        }
        // Any other event type: ignored, keep waiting.
    }
}

int TGridView::reorderArrowHitTest(int x) const {
    if (reorderVisualPos_ < 0) return -2;
    auto vis = visibleDisplayOrder();
    int n = (int)vis.size();
    if (reorderVisualPos_ >= n) return -2; // stale (shouldn't happen, defensive)
    int pos = 0;
    for (int i = 0; i < reorderVisualPos_; i++) pos += column(vis[i]).width + kSeparatorWidth;
    int width = column(vis[reorderVisualPos_]).width;
    if (x < pos || x >= pos + width) return -2; // outside the highlighted column
    bool hasLeft = reorderVisualPos_ > 0;
    bool hasRight = reorderVisualPos_ < n - 1;
    if (hasLeft && x == pos) return -1;
    if (hasRight && x == pos + width - 1) return 1;
    return 0; // inside the column, but not on a marker
}

void TGridView::refresh() {
    rows_->setRange((short)rowCount_);
    if (rows_->focused >= rows_->range && rows_->range > 0)
        rows_->focusItem(rows_->range - 1);
    rows_->drawView();
    header_->drawView();
}

int TGridView::focusedRow() const { return rows_->focused; }

void TGridView::focusRow(int row) { rows_->focusItem((short)row); }

void TGridView::relayout() {
    header_->drawView();
    rows_->drawView();
}

void TGridView::handleEvent(TEvent& event) {
    TGroup::handleEvent(event);
    // TListViewer::selectItem() sends this broadcast to its own owner
    // (this group) on double-click / Enter (see tlstview.cpp).
    if (event.what == evBroadcast &&
        event.message.command == cmListItemSelected &&
        event.message.infoPtr == rows_) {
        if (onRowActivate_) onRowActivate_(rows_->focused);
        clearEvent(event);
    }
}
