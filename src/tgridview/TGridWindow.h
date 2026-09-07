#pragma once

// TGridWindow — a plain TWindow that hosts a single TGridView, filling
// its client area. Using it is optional: TGridView is a normal TView
// and can be inserted into any window you already have. This exists
// only because "a window with a grid in it" is such a common shape
// that it's worth not rewriting the header/scrollbar/rows layout code
// (already inside TGridView) plus window-flag boilerplate every time.

#define Uses_TWindow
#include <tvision/tv.h>
#include "TGridView.h"

class TGridWindow : public TWindow {
public:
    // `fullScreen` mirrors the two ways this app's own torrent list has
    // been used: `true` gives you the locked, always-maximized main-
    // window behavior (flags = 0 — no move/resize/zoom/close, same
    // reasoning as this project's TorrentListWindow: it's meant to
    // always occupy the whole desktop); `false` gives you an ordinary
    // MDI child window (movable, resizable, closable, zoomable — ready
    // to sit alongside other windows on the desktop).
    TGridWindow(const TRect& bounds, TStringView title, bool fullScreen,
                ushort gridOptions = gvNone);

    TGridView* grid() const { return grid_; }

private:
    TGridView* grid_;
};
