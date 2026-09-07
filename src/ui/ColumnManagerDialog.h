#pragma once

#define Uses_TDialog
#include <tvision/tv.h>

class TorrentListWindow;

// A single consolidated place to resize, reorder, and show/hide the
// torrent list's columns — replacing what used to be three separate
// entry points (a "Resize columns" submenu, an "Order columns"
// submenu, and a standalone "Columns..." checkbox dialog) with one.
//
// Built as a small "meta" grid: one row per real column (Name, Done,
// Size, ...), showing its label/current width/visibility, using
// TGridView on itself to display that — a nice proof that the generic
// widget holds up being used this way too. Selecting a row and
// pressing "Resize" or "Move" calls straight into `target`'s own
// startColumnResize()/startColumnReorder() (the exact same interactive
// Left/Right/Enter/Esc loops the header's mouse/keyboard entry points
// already use), rather than reimplementing that logic here — so this
// window is really just a different way to pick which column those
// already-built interactions apply to, plus a toggle for visibility.
//
// Changes apply directly to `target` as they're made (there's no
// separate "confirm" step — each individual resize/reorder already has
// its own Enter/Esc, and toggling visibility is immediate), so the only
// button here besides "Reset" and "Close" is what's needed to pick a
// row and act on it.
TDialog* createColumnManagerDialog(TorrentListWindow* target);
