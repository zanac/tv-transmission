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
one thing it structurally can't do.** A `sortable` column (the
default) shows a dedicated, single-character hotspot at the end of its
own width — "□" normally, "^"/"v" once it's the active sort column —
and clicking *that specific character* toggles ascending/descending (or
selects the column ascending, if it wasn't already the sort column),
drawing the updated indicator, all without any code in the owner. It's
a deliberately separate hotspot from the rest of the column's name,
not a click-anywhere-on-the-header affordance: with `gvReorderableColumns`
also on, a plain click anywhere else on the name does nothing on its
own (see the reordering entry below for why sorting and reordering
can't both react to an ordinary click there). What the grid can't do is
reorder rows, because it doesn't own row data at all (see the
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

**A double-click's first, ordinary mouse-down used to fire the sort
toggle before the second one even arrived to signal a reorder.**
tvision delivers a double-click as two separate `evMouseDown` events —
a ordinary one, then a second one carrying `meDoubleClick` — so at the
moment the first lands there's no way yet to tell it's about to become
a double-click. When both sorting and reordering listened for "any
click on the column," the first click always sorted and the second
always reordered, on the same intended gesture. Fixing this is why
sorting moved to its own dedicated glyph (see above) instead of firing
on any click within the column: the glyph is checked first, unconditionally
of the double-click flag, so clicking it is sort-only regardless of
which half of a double-click it happens to be, and a plain click
anywhere else in the column now does nothing at all — only a
double-click there enters reorder mode. The two gestures no longer
share a hotspot, so they can't collide.

**Both markers/the glyph are reserved at fixed positions, not appended
and left to `fitToWidth()`'s truncation.** The original implementation
built each header cell as one string — label, then " ^"/" v", then
"<"/">" — and fit the whole thing into the column's width, truncating
whatever didn't fit. Since truncation cuts from the end, a label long
enough to fill the column would silently drop the *trailing* character
first — almost always the ">" marker, rarely the leading "<" — leaving
it undrawn while the hit-test still expected it at the assumed
position. Fixed by computing each cell's layout explicitly: the
marker/glyph occupies its exact reserved character(s) (first for "<",
last for ">" or the sort glyph), and the label is fit into whatever
width remains between them — never at risk of silently vanishing
regardless of how long the label is. Verified with a column name long
enough to have triggered the old truncation: the right-hand marker's
fixed position still registers a click correctly every time.

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

**The header is 2 rows tall: column labels on the first, a full "="
rule line on the second**, in the same header color — separating the
labels from the actual data rows below, the way a printed table's
header rule would. Every row/height calculation in this file accounts
for the extra row automatically (the header view itself, the rows
view's and scrollbar's starting position); a click landing on the rule
row is explicitly ignored by the header's own `handleEvent()` rather
than falling through to whichever column's hit-test happens to match
that x position, so it can never be mistaken for a click on the sort
glyph or a reorder marker one row up. No option to turn the rule line
off currently; it's a small enough cosmetic default that it didn't seem
worth one.

**Horizontal scrolling** appears automatically once the visible
columns' total width exceeds the view — a second `TScrollBar` along the
bottom, the header and the rows both reading a shared offset from it
when they draw. The two have to stay in sync (the header's labels would
stop lining up with the rows' data the moment either scrolled
independently of the other): the rows are a `TListViewer`, which
already reacts to its own horizontal scrollbar changing and redraws
itself — that's free — but the header isn't one, so `TGridView`'s own
`handleEvent()` catches the same `cmScrollBarChanged` broadcast and
redraws it too. Every existing hit-test (`columnAtX()`, the sort glyph,
the resize separator, the reorder markers) already worked in *content*-
relative coordinates rather than screen-relative ones, so none of them
needed touching — only one conversion, from the mouse's screen-relative
position to content-relative, added once at the top of the header's
`handleEvent()`.

The one genuinely fiddly part was the left edge. The right edge already
clips safely on its own — `TDrawBuffer`'s fixed-size buffer just
truncates whatever doesn't fit, the same reason extra columns used to
silently not appear at all before this widget could scroll — but the
left edge can't use the same trick, because `TDrawBuffer::moveStr()`'s
indent parameter is a `ushort`, which has no way to represent a
negative position for a column partially scrolled past. Clipping the
*string* instead of the position sidesteps that: `skipLeadingUtf8()` (a
codepoint-aware mirror of the `truncateUtf8()` already used for the
right edge, for the same reason — a byte-based clip could land
mid-character on non-ASCII content) drops however many leading display
columns have scrolled off, and the shortened text is drawn starting at
indent 0 instead.

**The scrollbar itself is hidden — and its row handed back to the rows
and vertical scrollbar, which grow to fill it — whenever there's
nothing to scroll**, rather than always reserving a row for a control
that would have nothing to do. `relayout()` checks this on every call
(so on every column add/remove/resize/reorder/show-hide, the same
things that already trigger it for other reasons) and grows or shrinks
`rows_`/`scrollBar_` by exactly the one row the horizontal scrollbar
needs, in whichever direction the content-vs-viewport comparison just
flipped.

That alone turned out not to be reliable enough: `relayout()` correctly
sets the hidden state right after construction, but tvision's own
view-insertion internals (`TGroup::insertBefore()`'s exposure cascade,
most likely — the exact mechanism wasn't traced down further once a
solid fix existed) can flip a child's visibility independently of this
widget's own `hide()`/`show()` calls once the grid is actually inserted
into a live desktop, with no `relayout()` call happening in between to
catch it. First fixed by not depending on the transition happening
exactly once: the header's own `draw()` re-validates and corrects the
scrollbar's visibility every time it draws, not only when `relayout()`
runs.

That fix introduced a worse problem of its own: it decided WHETHER to
resize by re-reading the scrollbar's own (externally-flippable)
visibility state, so if something kept flipping that state back between
one draw and the next, every single draw would see a "mismatch" again
and resize `rows_` by another row — not just once. Repeated column
visibility toggling could grow the rows area without bound, eventually
overlapping whatever sits below the grid entirely (the host
application's own status line, in TV Transmission's case). The real
fix: never read that external state for the resize decision at all. A
private `hScrollBarRowReserved_` flag — updated only by this widget's
own code, never touched by anything outside it — is the sole authority
for whether a resize happens; the scrollbar's own `show()`/`hide()`
call is still repeated unconditionally on every `draw()` so its on-
screen appearance keeps getting corrected, but that call no longer
feeds back into the resize decision. Decoupling "does this look right"
from "do we resize" is what stops the two from re-triggering each
other.

**Two ways to host it**: `TGridView` is a normal `TView` (a `TGroup`,
specifically) and can be `insert()`-ed into any window you already
have. `TGridWindow` exists only because "a window with nothing but a
grid in it" is such a common shape that it's worth not repeating the
header/scrollbar/rows layout wiring and window-flag boilerplate every
time. Its `fullScreen` argument mirrors two of the ways TV
Transmission's own torrent list has been used: `true` for a locked,
always-maximized window (`flags = 0` — no move/resize/zoom/close);
`false` for an ordinary MDI child window. A separate `closable`
argument (default `true`, ignored when `fullScreen` is `true` since
`flags = 0` already covers it) exists for exactly the case those two
alone don't: an MDI window the *user* still shouldn't be able to
close — TV Transmission's own torrent-list windows, one per
configured server, meant to always stay open for as long as that
server is configured and only ever closed programmatically (from
elsewhere removing that server), never from the window itself.

**A third piece, `TGridColumnManagerDialog`, is a ready-made "resize/
move/show/hide columns" window for any `TGridView`** — started out as
something specific to TV Transmission's own torrent list, then got
generalized here once it turned out almost nothing about it actually
was: the resize/move/toggle-visible actions were already one-line calls
into `TGridView`'s own public API, and the meta-grid's own "Column"
label was reading from an unnecessary local copy instead of
`column(i).header` directly. The one real gap was "Reset" — nothing
tracked what a column's default width/visibility had been once changed
— filled by adding `resetColumns()` to `TGridView` itself: a
`defaultColumns_` snapshot taken alongside `columns_` at the exact
moment each column is `addColumn()`-ed, restored (width, visibility,
and display order back to identity) on demand. Since this module has no
dependency on any particular app's translation system, the dialog's
own text (title, column headers, button labels) is a small
`TGridColumnManagerLabels` struct with plain-English defaults,
overridable by the caller — see TV Transmission's own `App.cpp` for an
example of building one from an app's existing translated strings right
before calling `createColumnManagerDialog()`.

**Multiple selection is opt-in** via `gvMultiSelect` — off by default, so
a grid built without it (a column manager's own meta-grid, a tracker
list) behaves exactly as it always did, paying nothing for the extra
hit-testing. Once a grid has it, `enterSelectionMode()` — called
directly (e.g. from a menu command, for keyboard-only use: Space then
toggles whichever row is focused) or via a 3-second press-and-hold on a
row — shows a leftmost `[X]`/`[ ]` checkbox column, drawn the same
fixed-and-never-scrolled way the checkbox column always is (see
`kSelectionColumnWidth` and `drawScrolled()`'s own `prefixWidth`
parameter, which every column and separator already goes through). Any
click on a row toggles it — not just a precise hit on the tiny `[X]`
itself — since requiring pixel-perfect clicks on something this small
would work against the point of making batch-selecting easier.
`selectedRows()` returns every checked row's index; `setRowCount()`
keeps the underlying tracking vector in lockstep with the row count
whenever it changes while selection mode is active, so a mid-selection
data refresh doesn't leave it too short.

The long-press gesture needed a different mechanism than the drag-
tracking `dragResize()` already uses elsewhere in this file:
`TView::mouseEvent()` only returns once an event matching its own mask
actually occurs, so a mask built for movement would leave it blocked
indefinitely on a hold that never moves — exactly the case that
matters here. Calling `getEvent()` directly instead relies on
`TProgram::getEvent()`'s own wait timeout, which still returns
periodically with `evNothing` even with nothing happening at all (the
same path `idle()` runs on) — that's what lets an elapsed-time check
actually get a chance to run rather than blocking on real input that
may never come before release. Esc and Enter both leave selection mode
too (in addition to the menu entry a caller wires up itself — see
TV Transmission's own "Select Multiple"), without toggling the focused
row on the way out.

**Double-click can be column-aware**, via `CellActivateFn` — set with
`setCellActivateCallback()`, alongside the existing `RowActivateFn`
(`setRowActivateCallback()`), not in place of it. The two exist for
different reasons: `RowActivateFn` is driven by `TListViewer`'s own
generic `cmListItemSelected` broadcast (fired for Enter too, not just a
mouse double-click), which only ever carries a row — nothing at that
point in tvision's own handling knows *where* a click landed, only
which row is now focused. `CellActivateFn` is fired directly from the
rows view's own mouse handling instead, where the click's exact
position is still available, specifically for a genuine mouse
double-click (`meDoubleClick`) and only outside selection mode (a
double-click there already means something else — see above).

`CellActivateFn` returns `bool` — whether it considered that
double-click its own concern. Returning `true` consumes the event
outright, so `RowActivateFn` (if also set, e.g. TV Transmission's main
list opening details on any other column's double-click) doesn't *also*
fire for a column meant to do something else instead. Returning `false`
leaves the event alone, falling through to `TListViewer::handleEvent()`
exactly as if `CellActivateFn` had never been set for that column — the
right choice for any column this callback doesn't specifically care
about, letting whatever `RowActivateFn` would otherwise do keep working
unchanged. Falling through that way is safe even for an ordinary
double-click, unlike a plain single click: that base class's own
press-tracking loop (see `tlstview.cpp`) checks for `meDoubleClick`
*before* ever calling `mouseEvent()`, so an event that already carries
the flag on arrival exits the loop immediately rather than blocking —
confirmed by reading tvision's own source rather than assumed. Column
hit-testing (`columnAtX()`) is shared between the header and the rows
view rather than duplicated a second time for this — it's a method on
`TGridView` itself now, not a private detail of the header.

**A pre-flagged double-click can also reach the long-press watch** (see
above) if `CellActivateFn` doesn't claim it — `multiSelectCapable()`
alone used to be enough to start `watchForLongPress()`, with no check
for whether this particular `evMouseDown` was already the second half
of a double-click rather than a fresh press. That watch's own
`getEvent()` loop then has no real release to find for a button that,
as far as this specific event is concerned, was never freshly pressed —
blocking indefinitely with no live event queue behind it (a synthetic
test), or behaving unpredictably against whatever real release does
eventually surface (a live app). Excluded now by adding `meDoubleClick`
to that branch's own condition, alongside `multiSelectCapable()`.

## What this does *not* do (yet)

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

`resetColumns()` and `TGridColumnManagerDialog` are both verified
directly too: width, visibility, *and* order are changed away from
their defaults first, then `resetColumns()` is confirmed to restore the
exact original values for all three at once, not some state left over
from whatever was changed most recently. The dialog itself is exercised
against a plain `TGridView` with no other project code involved at
all — confirming it's genuinely not coupled to TV Transmission
specifically — checking that its meta-grid reads column labels live
from the real grid (not a stale local copy), that custom
`TGridColumnManagerLabels` are actually used when supplied and the
English defaults apply cleanly when they're not, and that its own
Reset button, triggered through the dialog's normal command dispatch
rather than calling `resetColumns()` directly, produces the same
restored values on the underlying grid. Its "Visible" marker showing
`[X]`/`[ ]` and a double-click on a row toggling it are both checked
directly too, including that a *second* double-click flips it back —
a real toggle, not a one-way action.

Horizontal scrolling is verified beyond "it visibly looks scrolled":
the computed range is exactly zero once every column already fits (no
behavior change for the common case) and the exact expected non-zero
value once content exceeds the view; a real running instance, captured
off an actual rendered terminal screen, shows the header and rows
staying character-for-character in sync as the offset changes, with a
multi-byte UTF-8 label correctly clipped mid-string at the left edge
rather than corrupted or crashing. The check that matters most,
though, isn't that anything *looks* right — it's that after scrolling,
a click at the position where a column's sort glyph now sits actually
sorts *that* column, confirmed by which column index the sort callback
receives, not just that a click did something.

Hiding the scrollbar when it's not needed is verified the same
directly-inspected way, in both directions: a grid whose columns
already fit starts with it hidden and the row reclaimed; resizing a
column until content exceeds the view flips it to shown with the row
given back, confirmed by reading the scrollbar's own visibility state
and range rather than just eyeballing whether content looks clipped.
That covers `relayout()`'s own transition logic directly, but missed
the actual reported bug — reading the same state again once the grid is
inserted into a live desktop (not just right after construction) is
what caught tvision's own insertion machinery quietly re-showing it.
With the fix (re-validated on every `draw()`, not only in `relayout()`)
in place, the real application itself — not a synthetic reproduction —
confirms it end to end: a terminal too narrow for a given set of
columns shows the scrollbar, a wide enough one doesn't, exercised by
nothing more than the application's own ordinary draw cycle.

That fix's own regression — the rows area growing without bound from
resizing on every draw rather than once per transition — gets the most
rigorous check in this module: 50 consecutive redraws with nothing
changed leave the row count exactly unchanged, not just "still looks
about right"; 20 rounds of toggling a column visible and hidden land on
the exact same two row-count values every time, with zero drift in
either direction; and in the real application, with real `refresh()`
calls rather than a single synthetic check, 30 rapid toggles leave the
window's own border and the application's status line exactly where
they belong.

Multiple selection is checked in the same layered order it was built:
the widget's own mechanics first, entirely on their own — a grid built
without `gvMultiSelect` leaves every selection API a safe no-op;
`enterSelectionMode()`/`toggleRowSelected()`/`selectedRows()` toggle and
report correctly on their own, including out-of-range indices being
silently ignored rather than corrupting anything; the checkbox column
stays fixed in place through horizontal scrolling, confirmed off a real
rendered screen rather than just trusting the coordinate math; and the
3-second gesture is timed against a real terminal with genuine elapsed
time — a hold past 3 seconds shows the checkbox column with that row
already checked, a short click well under the threshold shows neither.
One thing worth noting about testing mouse interaction here at all:
`TListViewer`'s own base handling of a click can block waiting for a
release event to arrive through tvision's real event queue, which a
synthetic single `handleEvent()` call outside a live application has no
way to supply — confirmed by checking that this widget's own routing
logic (which row a click lands on) had already run and updated state
correctly *before* that block occurred, rather than treating the hang
itself as a failure.
