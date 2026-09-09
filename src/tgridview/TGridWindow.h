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
    // `fullScreen` mirrors two of the ways this app's own torrent list
    // has been used: `true` gives you the locked, always-maximized
    // main-window behavior (flags = 0 — no move/resize/zoom/close);
    // `false` gives you an ordinary MDI child window (movable,
    // resizable, zoomable — ready to sit alongside other windows on
    // the desktop).
    //
    // `closable` is a SEPARATE axis from `fullScreen`, not a special
    // case of it: an MDI window (fullScreen=false) can still be one the
    // user isn't meant to close — e.g. this app's own per-server
    // torrent-list windows, which stay open for as long as that server
    // is configured, only closing when the Connection dialog's own
    // "[-]" removes it, not from the window itself. Ignored when
    // fullScreen is true, since flags = 0 already strips wfClose (and
    // everything else) unconditionally there.
    TGridWindow(const TRect& bounds, TStringView title, bool fullScreen,
                ushort gridOptions = gvNone, bool closable = true);

    TGridView* grid() const { return grid_; }

private:
    TGridView* grid_;
};
