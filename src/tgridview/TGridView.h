#pragma once

// TGridView — a generic, dynamic-column list view for Turbo Vision.
//
// Self-contained on purpose: this file and TGridView.cpp depend on
// nothing outside tvision itself and the C++ standard library — no
// project-specific types, no other headers from this app. That's so
// the whole tgridview/ folder can be copied into a different project
// wholesale and just work.
//
// See tgridview/README.md for the full design writeup (why a
// callback-based data source instead of the grid owning row data,
// exactly how column resizing behaves, etc.).

#define Uses_TGroup
#define Uses_TRect
#define Uses_TEvent
#include <tvision/tv.h>

#include <functional>
#include <string>
#include <vector>

// One column's definition. Plain data — TGridView copies these in and
// out; there's no hidden per-column state elsewhere.
struct TGridColumn {
    enum class Align { Left, Right };

    std::string header;
    int width = 10;      // current width, in character columns
    int minWidth = 3;     // floor enforced by setColumnWidth() and dragging
    bool resizable = true; // per-column override; see gvResizableColumns below
    bool sortable = true;  // whether clicking this column's header sorts by it
    bool movable = true;   // per-column override; see gvReorderableColumns below
    bool visible = true;   // hidden columns take no screen space at all — see
                            // setColumnVisible() below — but keep their place in
                            // columnOrder() so re-showing one doesn't lose where
                            // it was
    Align align = Align::Left;
};

// TGridView's own options bitmask (passed to the constructor).
enum TGridOptions : ushort {
    gvNone = 0x0000,
    // Lets the user drag the separator between two column headers with
    // the mouse to resize the column on its left — see README.md for
    // exactly how the drag hotspot and live feedback work. Off by
    // default: a grid that's just displaying fixed data has no reason
    // to pay for hit-testing every header click.
    gvResizableColumns = 0x0001,
    // Lets the user double-click a column's header to enter a reorder
    // mode: "<"/">" markers appear on the column (whichever apply — none
    // at either edge of the grid), and further clicks on them, or the
    // Left/Right arrow keys, or startKeyboardReorder(), move it one
    // position at a time; any other click, or Enter, confirms; Esc
    // restores the order the grid had when the mode was entered. See
    // README.md for why this needed a real visual-position/logical-
    // index split throughout the widget.
    gvReorderableColumns = 0x0002,
};

class TGridView : public TGroup {
public:
    // `options` is a bitwise-OR of TGridOptions. Fills the whole of
    // `bounds` — header row on top, scrollable rows and a vertical
    // scrollbar below, exactly like the fixed-column list this was
    // generalized from (see the project's TorrentListWindow for the
    // shape this replaces).
    TGridView(const TRect& bounds, ushort options = gvNone);

    // --- Column management — fully dynamic, at any time ---
    // Returns the new column's index.
    //
    // IMPORTANT: every index here (and in every callback below) is a
    // LOGICAL index — a column's stable identity, assigned once when
    // it's added and never changed by reordering. It is NOT the same as
    // where the column currently appears on screen (its VISUAL
    // position), which changes when the user reorders columns (see
    // gvReorderableColumns). This split exists so a caller's own
    // cellText()/cellBold()/etc. callbacks — which switch on `col` to
    // decide which field to return — keep working correctly no matter
    // how the user has rearranged the columns visually; see
    // columnOrder()/setColumnOrder() below for the actual visual
    // arrangement. Any structural change (addColumn(), insertColumn(),
    // removeColumn(), clearColumns()) resets the visual order back to
    // logical order (identity) — call setColumnOrder() again afterward
    // if you need to restore a specific arrangement.
    int addColumn(const TGridColumn& col);
    void insertColumn(int index, const TGridColumn& col);
    void removeColumn(int index);
    void clearColumns();
    int columnCount() const { return (int)columns_.size(); }
    TGridColumn& column(int index) { return columns_[index]; }
    const TGridColumn& column(int index) const { return columns_[index]; }
    void setColumnWidth(int index, int width); // clamped to that column's minWidth

    // Hides or shows a column — it keeps its width/position/all other
    // settings, it just takes no screen space and is skipped by every
    // visual operation (drawing, click hit-testing, resize, reorder)
    // while hidden. `index` is the LOGICAL index, same convention as
    // everything else here.
    void setColumnVisible(int index, bool visible);
    bool isColumnVisible(int index) const {
        return index >= 0 && index < (int)columns_.size() && columns_[index].visible;
    }

    // Restores every column's width and visibility to what they were
    // when addColumn()/insertColumn() first defined them, and the
    // display order back to identity — everything a "start over" button
    // would need, and nothing else (header text/align/sortable/
    // resizable/movable aren't things resetting layout typically means
    // to touch, so they're left alone). The snapshot this restores from
    // is taken automatically at the point each column is added; there's
    // nothing separate to set up for it.
    void resetColumns();

    // --- Data source ---
    // TGridView never owns or copies row data: it asks for exactly the
    // cell it's about to draw, when it's about to draw it. This avoids
    // formatting the whole table into strings up front on every
    // refresh (as the fixed-column version this replaces used to) and
    // means there's no separate "the grid's copy is stale" state to
    // manage — whatever the callback returns right now is what's shown.
    using CellTextFn    = std::function<std::string(int row, int col)>;
    using RowColorFn     = std::function<TColorAttr(int row, bool focused)>; // optional
    using CellBoldFn      = std::function<bool(int row, int col)>;             // optional
    using RowActivateFn  = std::function<void(int row)>;               // double-click / Enter
    using RowContextFn   = std::function<void(int row, TPoint screenPos)>; // right-click
    using RowFocusFn      = std::function<void(int row)>; // focused row changed (arrow keys, click, focusRow())

    // Clicking a sortable column's header (see TGridColumn::sortable)
    // toggles ascending/descending if it's already the active sort
    // column, or switches to it ascending otherwise — handled entirely
    // inside TGridView, including drawing the "^"/"v" indicator on the
    // active column's header at draw time (never stored into
    // TGridColumn::header itself, so nothing needs to strip/reapply it
    // when column labels change, e.g. on a language switch). The grid
    // has no idea how to actually reorder rows (it doesn't own row
    // data — see the "Data source" section above), so this callback is
    // where the owner does that and calls setRowCount()/refresh().
    using SortChangedFn = std::function<void(int col, bool ascending)>;

    void setRowCount(int count);
    void setCellTextCallback(CellTextFn fn);
    void setRowColorCallback(RowColorFn fn);
    void setCellBoldCallback(CellBoldFn fn);
    void setRowActivateCallback(RowActivateFn fn);
    void setRowContextCallback(RowContextFn fn);
    void setRowFocusCallback(RowFocusFn fn);
    void setSortChangedCallback(SortChangedFn fn);

    // Sets which column shows the sort indicator and in which
    // direction, WITHOUT invoking setSortChangedCallback()'s callback —
    // for restoring a previously-chosen (e.g. persisted) sort at
    // startup, where the owner's data is already in that order and
    // doesn't need to be told to re-sort itself.
    void setSortIndicator(int col, bool ascending);
    int sortColumn() const { return sortColumn_; }
    bool sortAscending() const { return sortAscending_; }

    // Keyboard-driven equivalent of dragging the mouse resize handle —
    // meant to be triggered from outside the grid (e.g. a menu item
    // picking which column to resize), not from within the header's own
    // event handling. Left/Right arrows shrink/grow the column live
    // (redrawn after every press), Enter confirms and returns, Esc
    // cancels and restores the width the column had when this was
    // called. Blocks until confirmed or cancelled — same "pump events in
    // a loop until some condition" shape as the mouse-drag resize (see
    // README.md), just keyboard-driven instead of mouse-driven. No-op if
    // `col` is out of range or that column isn't resizable (see
    // TGridColumn::resizable) — a menu item invoking this on a
    // fixed-width column would otherwise silently "work" without doing
    // anything visible, which is worse than not entering the mode at
    // all.
    void startKeyboardResize(int col);

    // --- Column reordering (see gvReorderableColumns above) ---
    // The visual arrangement: columnOrder()[visualPosition] is the
    // LOGICAL index of whichever column currently sits there. Identity
    // ([0, 1, 2, ...]) until reordered.
    std::vector<int> columnOrder() const { return displayOrder_; }

    // Restores a previously-chosen (e.g. persisted) order WITHOUT
    // invoking setColumnOrderChangedCallback()'s callback — same
    // reasoning as setSortIndicator(). Ignored, falling back to
    // identity order, if `order` isn't a permutation of exactly
    // [0, columnCount()) — a mismatched size (e.g. a settings file from
    // before a column was added) or any invalid/duplicate index would
    // otherwise leave the grid in a broken state.
    void setColumnOrder(const std::vector<int>& order);

    using ColumnOrderChangedFn = std::function<void(const std::vector<int>&)>;
    // Fired once, when a reorder is confirmed (Enter, or any click that
    // isn't on a "<"/">" marker) — not on every single swap while
    // dragging through positions, the same way setSortChangedCallback()
    // fires once per click rather than per intermediate state.
    void setColumnOrderChangedCallback(ColumnOrderChangedFn fn);

    // columnOrder() filtered down to just the visible (see
    // TGridColumn::visible / setColumnVisible()) columns' logical
    // indices, in visual order — what's actually drawn, left to right.
    std::vector<int> visibleDisplayOrder() const;

    // Keyboard-driven column reordering, analogous to
    // startKeyboardResize() above — meant to be triggered from outside
    // the grid (e.g. a menu item picking which column to move), not
    // from within the header's own event handling. `col` is that
    // column's LOGICAL index; Left/Right move it one visual position at
    // a time (live, redrawn after every press — also usable via mouse
    // clicks on the "<"/">" markers this draws on the column while
    // active), Enter confirms, Esc restores the order the grid had when
    // this was called. No-op (never enters the loop) if `col` is out of
    // range or that column has movable=false.
    void startKeyboardReorder(int col);

    // Call after anything that changes what a row/cell would display
    // (row count, or the underlying data the callbacks read from) —
    // same idea as TListViewer::setRange() + drawView(), just named for
    // what a caller here is actually doing.
    void refresh();

    int focusedRow() const;
    void focusRow(int row);
    int rowCount() const { return rowCount_; }

    // How many character columns of content are currently scrolled off
    // the left edge — 0 until there are more visible columns than fit
    // in the view and the user drags/clicks the horizontal scrollbar.
    // Read by the header and rows views to know where to start drawing
    // from; not meant to be set directly (drive the scrollbar itself,
    // via mouse/keyboard — same as the vertical one).
    int horizontalScrollOffset() const;

    void handleEvent(TEvent& event) override; // catches double-click from the rows view, and the horizontal scrollbar's own broadcast

private:
    friend class TGridHeaderView;
    friend class TGridRowsView;

    void relayout(); // repositions header/rows/scrollbar after a bounds or column change
    // Resizes rows_/scrollBar_ by exactly one row, in whichever
    // direction hScrollBarRowReserved_ needs to change, and re-applies
    // hScrollBar_'s own show()/hide() to match. Safe to call on every
    // relayout() *and* every draw() (see TGridHeaderView::draw()
    // calling this first, before actually drawing, so a visible glitch
    // is corrected before it would ever reach the screen) BECAUSE the
    // resize decision is driven entirely by hScrollBarRowReserved_, our
    // own tracked flag — never by re-reading hScrollBar_->state, which
    // tvision's own view-insertion machinery (TGroup::insertBefore()'s
    // exposure cascade, most likely — tracing it down further wasn't
    // worth it once this fix existed) can flip independently of this
    // widget's own calls. The show()/hide() call is repeated
    // unconditionally on every invocation, harmlessly, precisely so
    // that repeated external interference keeps getting corrected
    // visually WITHOUT ever re-triggering the resize a second time for
    // the same already-handled transition.
    void updateHScrollBarVisibility();
    // Total width, in character columns, of every VISIBLE column plus
    // the single-character separators between them — what the
    // horizontal scrollbar's range is computed from in relayout().
    int totalContentWidth() const;
    int visualPositionOf(int logicalCol) const; // position within displayOrder_, -1 if not found
    // Position within the VISIBLE-only subset of displayOrder_ (what
    // every visual operation — drawing, hit-testing, reorder — actually
    // iterates over), -1 if not found or not visible. See
    // visibleDisplayOrder() in the .cpp for why hidden columns are
    // filtered out entirely rather than drawn at zero width inline.
    int visiblePositionOf(int logicalCol) const;
    void resetColumnOrder(); // identity order — called after any structural column change
    // Shared by the double-click (mouse) and startKeyboardReorder()
    // (keyboard) entry points — see the big comment on
    // gvReorderableColumns for why one loop handles both input methods.
    void runReorderLoop(int startVisualPos);
    // x is CONTENT-relative (see TGridHeaderView::handleEvent()'s own
    // conversion from screen-relative local.x, which accounts for
    // horizontal scrolling). Returns -1 for a click on the "<" marker,
    // +1 for ">", 0 for anywhere else within the currently-highlighted
    // column's own span, or -2 for a click outside it entirely. Used
    // only while reorderVisualPos_ >= 0.
    int reorderArrowHitTest(int x) const;

    std::vector<TGridColumn> columns_;
    // Snapshot of each column exactly as addColumn()/insertColumn() was
    // given it — kept in lockstep (same indices) with columns_ itself,
    // purely so resetColumns() has something to restore width/visible
    // from. Never read anywhere else.
    std::vector<TGridColumn> defaultColumns_;
    std::vector<int> displayOrder_; // displayOrder_[visualPos] = logical column index
    ushort options_;
    int rowCount_ = 0;
    int sortColumn_ = -1;    // -1 = no column currently shows an indicator
    bool sortAscending_ = true;
    int reorderVisualPos_ = -1; // -1 = not currently in a reorder mode; else the
                                 // visual position currently showing "<"/">" markers

    CellTextFn cellText_;
    RowColorFn rowColor_;
    CellBoldFn cellBold_;
    RowActivateFn onRowActivate_;
    RowContextFn onRowContext_;
    RowFocusFn onRowFocus_;
    SortChangedFn onSortChanged_;
    ColumnOrderChangedFn onColumnOrderChanged_;

    class TGridHeaderView* header_ = nullptr;
    class TGridRowsView* rows_ = nullptr;
    class TScrollBar* scrollBar_ = nullptr;
    class TScrollBar* hScrollBar_ = nullptr;
    // Whether rows_/scrollBar_ currently have their bottom row SHRUNK
    // to make room for hScrollBar_ — the sole authority for whether
    // updateHScrollBarVisibility() resizes them again. Starts true:
    // the constructor's own initial rowsRect/scrollRect already assume
    // the row is reserved (see TGridView::TGridView()), so this must
    // match that from the outset — starting it false would make the
    // very first correction shrink an already-shrunk rows_ by a second
    // row instead of recognizing nothing has changed yet. Deliberately
    // NEVER read from hScrollBar_->state itself: something outside
    // this widget's own control can flip that independently (see
    // updateHScrollBarVisibility()'s own doc comment), and driving a
    // resize off a value that can silently disagree with reality would
    // mean every redraw re-triggers the same "needs resizing" branch —
    // growing (or shrinking) rows_ by another row every single time,
    // unboundedly, rather than exactly once per actual transition.
    bool hScrollBarRowReserved_ = true;
};
