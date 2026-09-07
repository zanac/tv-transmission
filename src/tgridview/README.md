# TGridView

A generic, dynamic-column list view for [Turbo Vision](https://github.com/magiblot/tvision).
Grew out of TV Transmission's own fixed-column torrent list, generalized
for reuse in other Turbo Vision projects.

**Self-contained**: this folder depends only on tvision itself and the
C++ standard library. Nothing here includes anything else from
TV Transmission. Copy the whole `tgridview/` folder into another
project's source tree, add the two `.cpp` files to its build, and it
works.

## Files

- `TGridView.h` / `.cpp` — the widget itself: dynamic columns, a
  callback-based data source, optional mouse-driven column resizing.
- `TGridWindow.h` / `.cpp` — an optional convenience `TWindow` that
  hosts one `TGridView` filling its client area, with a `fullScreen`
  flag choosing between a locked always-maximized window and an
  ordinary movable/resizable MDI child window.

## Quick start

```cpp
#include "tgridview/TGridWindow.h"

// One line per torrent; three columns.
auto* win = new TGridWindow(bounds, "Torrents", /*fullScreen=*/true);
TGridView* grid = win->grid();

grid->addColumn({"Name", 30});
grid->addColumn({"Size", 10, /*minWidth=*/3, /*resizable=*/true, /*sortable=*/true,
                  /*movable=*/true, /*visible=*/true, TGridColumn::Align::Right});
grid->addColumn({"Status", 15});

grid->setRowCount((int)torrents.size());
grid->setCellTextCallback([&](int row, int col) -> std::string {
    switch (col) {
        case 0: return torrents[row].name;
        case 1: return formatSize(torrents[row].sizeBytes);
        case 2: return statusName(torrents[row].status);
    }
    return "";
});
grid->setRowActivateCallback([&](int row) { showDetails(torrents[row]); });

// Clicking a sortable column's header toggles ascending/descending and
// draws the "^"/"v" indicator automatically — this callback only needs
// to actually reorder `torrents` and tell the grid how many rows there
// are now (still the same count here, just re-sort() it before this).
grid->setSortChangedCallback([&](int col, bool ascending) {
    sortTorrentsBy(torrents, col, ascending); // your own comparator
    grid->refresh();
});

grid->refresh();
```

Whenever the underlying data changes (a fresh RPC fetch, a filter
applied, whatever), update `setRowCount()` if the count changed and
call `refresh()`. The grid never copies row data — it calls
`cellText()` again for exactly the cells it's about to draw, so
there's nothing to keep in sync beyond the count.

## Design notes

**Callback-based data source, not an internal `vector<vector<string>>`.**
The first design considered was "the grid owns a table of strings,
call `setData()` to update it." Rejected: that means formatting every
visible cell into a string up front on every refresh, even the ones
scrolled out of view, and creates a second copy of the data that has
to be kept in sync with whatever the caller's own source of truth is.
A callback (`row, col -> string`) asks for exactly what's about to be
drawn, when it's about to be drawn — no formatting work wasted on
off-screen rows, no stale-copy bugs possible.

**Columns are plain data (`TGridColumn`), added/removed/reordered
through the grid at any time** — `addColumn()`, `insertColumn()`,
`removeColumn()`, `clearColumns()`. Nothing about the widget's
internals assumes a fixed column count decided at construction.

**Column resizing (`gvResizableColumns`) drags the column to the left
of the separator, not a proportional split.** Dragging the boundary
between column N and N+1 changes only column N's width; every column
after it shifts along with it, and the grid's total content width
changes rather than staying fixed. This was chosen over redistributing
the freed/needed space across the *other* columns because that would
mean every resize touches N columns' widths instead of one, which is
harder to reason about as a user ("why did resizing column 2 also
change column 5?") and harder to implement correctly. If the total
width ends up wider than the view, cells past the edge are simply
clipped — the same as any row's content that doesn't fit, no special
horizontal-scroll handling needed.

The drag itself uses tvision's own `mouseEvent(event, evMouseMove)` —
the same blocking-loop-until-mouseUp pattern `TView`'s own window
move/resize logic uses internally (see `tview.cpp`). The grab target is
the single-character separator between two column headers, drawn as
"│" specifically when `gvResizableColumns` is set, so there's a visible
cue for where to grab instead of an invisible one-column gap.

**Column resizing also has a keyboard-driven entry point**,
`startKeyboardResize(col)`, meant to be called from outside the grid
entirely (a picker of which column to resize is the intended use — see
TV Transmission's own "Settings → Manage columns..." window). Left/
Right shrink/grow the column live, Enter confirms, Esc cancels and
restores the width the column had when it was called. Built on the same
primitive as the mouse drag — `TView::getEvent()`, which `mouseEvent()`
itself calls internally — just pumping keyboard events instead of mouse
ones. No-ops immediately (never entering that loop at all) for an
out-of-range column index or one with `resizable = false`, rather than
entering an interactive mode that can't do anything.

**Sorting-on-click is handled entirely inside the grid, except for the
one thing it structurally can't do.** Clicking a `sortable` column
(the default) toggles ascending/descending if it's already the active
sort column, or selects it ascending otherwise, and draws the "^"/"v"
indicator — all without any code in the owner. What the grid can't do
is reorder rows, because it doesn't own row data at all (see the
callback-based data source above) — reordering `torrents` (or whatever
the owner's actual collection is) has to happen in
`setSortChangedCallback()`. The indicator itself is drawn at header-
draw time from `TGridView`'s own tracked `sortColumn()`/
`sortAscending()`, never stored into `TGridColumn::header` — so
changing a column's label (a language switch, for instance) never has
to remember to strip an old indicator before applying a new label.
`setSortIndicator()` sets that same state without invoking the
callback, for restoring a persisted sort at startup when the data is
already being loaded in that order and there's nothing to react to.

**Column reordering (`gvReorderableColumns`) required a real split
between a column's stable identity and where it's currently drawn.**
Double-clicking a `movable` column's header (or calling
`startKeyboardReorder(col)`) enters a mode where "<"/">" markers appear
next to it — whichever apply (none at either edge of the grid) — and
Left/Right (or clicking a marker) move it one position at a time, live;
Enter, or any click that isn't on a marker, confirms; Esc restores the
order the grid had when the mode was entered. This shares its event
loop (`runReorderLoop()`) between both entry points, handling mouse and
keyboard input side by side, so either works regardless of which one
started the mode.

The harder part wasn't the loop — it was `columnOrder()`, a
`displayOrder_` permutation mapping visual position to logical column
index, threaded through the header's drawing and hit-testing and the
rows' rendering. Every index a caller ever gets from this widget
(`cellText(row, col)`, `cellBold(row, col)`, the sort/resize APIs, an
`addColumn()` return value, ...) is that stable logical index, assigned
once and never changed by reordering — never the visual position, which
moves. A caller's own `cellText()` typically switches on `col` to
decide which field to return; if reordering changed what `col` meant,
every column after the one moved would start showing the wrong data.
`setColumnOrder()` restores a persisted arrangement the same way
`setSortIndicator()` does for sorting — without firing
`setColumnOrderChangedCallback()`'s callback — and rejects anything
that isn't exactly a permutation of `[0, columnCount())` (wrong size, an
out-of-range or duplicate index), falling back to identity order rather
than leaving some column drawable twice or not at all.

**Column visibility (`setColumnVisible()`) is the visual/logical split
pushed one step further.** A hidden column takes zero screen space and
is skipped entirely by drawing, hit-testing, resizing and reordering —
`visibleDisplayOrder()` (a fresh filter of `columnOrder()` down to just
the shown columns, computed on demand) is what every one of those paths
actually iterates. Hiding doesn't forget the column's position: it just
disappears from that filtered list, and re-showing it re-inserts it
wherever `columnOrder()` still remembers it belonged — no need to call
`setColumnOrder()` again. Reordering *while* some columns are hidden is
what made this genuinely tricky rather than a one-line filter: two
columns that are visual neighbors in `visibleDisplayOrder()` aren't
necessarily adjacent in the real (unfiltered) `displayOrder_`, if a
hidden column happens to sit between them — swapping them has to find
each one's *true* position first (`visualPositionOf()`), not just swap
two adjacent array slots, so a column can move past a hidden neighbor
exactly as if it weren't there, without disturbing where that hidden
column will reappear once shown again.

**Two ways to host it**: `TGridView` is a normal `TView` (a `TGroup`,
specifically) and can be `insert()`-ed into any window you already
have. `TGridWindow` exists only because "a window with nothing but a
grid in it" is such a common shape that it's worth not repeating the
header/scrollbar/rows layout wiring and window-flag boilerplate every
time. Its `fullScreen` argument mirrors the two ways TV Transmission's
own torrent list has been used: `true` for a locked, always-maximized
main window (`flags = 0` — no move/resize/zoom/close, matching that
project's `TorrentListWindow`); `false` for an ordinary MDI child
window.

## What this does *not* do (yet)

- **Horizontal scrolling.** If columns' total width exceeds the view,
  content past the right edge is clipped rather than scrolled into
  view. Fine for a handful of columns at reasonable widths; would need
  work for a genuinely wide table.
- **Per-cell custom widgets** (buttons, checkboxes inside a cell) —
  cells are text only, optionally bold (`setCellBoldCallback`) and
  optionally colored per row (`setRowColorCallback`), not per cell.
- **Persisting column widths/order/visibility** — left entirely to the
  embedding application (read `column(i).width`/`columnOrder()`/
  `isColumnVisible(i)` yourself and save them however you already save
  your own settings), so this widget has no file I/O of its own.

## Testing note

Column add/remove/insert, the callback-based data source, width
clamping (`setColumnWidth()`'s `minWidth` floor), the double-click
row-activation broadcast, sorting (a simulated click selecting an
unsorted column ascending, a second click on the same column flipping
to descending, and `setSortIndicator()` updating the displayed state
without firing the callback), and `startKeyboardResize()`'s guard
clauses (an out-of-range column index or a non-resizable one returns
immediately, verified with a timeout wrapped around the test
specifically to catch a regression that made it loop forever instead)
are all covered by tests that construct a `TGridView` and `TGridWindow`
directly and exercise them without a live terminal (see the project's
own test suite for examples of this pattern).

Reordering got a step further: a small test subclass overrides
`getEvent()` to feed `runReorderLoop()` a scripted sequence of key
events instead of pulling from a real terminal, which made the actual
interactive loop itself verifiable headless — not just its guard
clauses. That covers Right/Right/Enter landing on the expected final
order and firing the callback with it, Esc reverting with no callback
call, and Left at the first position being a no-op. The one thing that
still needs a real terminal is the visual/mouse side specifically —
actually clicking the "<"/">" markers or watching a column follow the
arrow keys on screen — the same limitation that applies to any
interactive mouse handling built on tvision. Also verified end to end,
against a real (mocked) data source: after reordering, a cell's content
stays correctly associated with its logical column rather than
silently shifting to whatever moved into that visual position.

Visibility (`setColumnVisible()`) is covered the same way: a hidden
column's `cellText()` callback is never even called during rendering,
`visibleDisplayOrder()` correctly excludes it while `columnOrder()`
(the full, unfiltered order) doesn't forget it, and re-showing it
restores its previous position. The scenario most likely to hide a
subtle bug — reordering two visible columns with a hidden one sitting
between them — is verified directly too: the swap lands on the correct
pair (skipping the hidden one, not swapping with it by mistake), and
showing the hidden column again afterward accounts for every column
exactly once, none duplicated or dropped.
