#include "TGridWindow.h"

TGridWindow::TGridWindow(const TRect& bounds, TStringView title, bool fullScreen,
                          ushort gridOptions)
    : TWindowInit(&TWindow::initFrame),
      TWindow(bounds, title, wnNoNumber) {
    if (fullScreen) {
        // Same reasoning as this project's TorrentListWindow: meant to
        // always occupy the whole desktop, so every window-management
        // flag (move/resize/zoom/close) is removed rather than just
        // disabled — wfClose in particular has to go because TWindow::
        // close() would destroy(this), leaving whoever holds this
        // pointer with a dangling reference on their next use of it.
        // ofTileable deliberately NOT set here: TDeskTop::tile()/
        // cascade() reposition via locate() regardless of the
        // wfMove/wfGrow flags just stripped above (those only gate
        // interactive keyboard/mouse move/resize, not programmatic
        // repositioning) — marking a flags=0 window tileable would let
        // a Tile/Cascade elsewhere on the desktop forcibly move or
        // resize it, breaking the "always fills the desktop" invariant
        // this branch exists for.
        flags = 0;
    } else {
        // The MDI case: free to be tiled/cascaded alongside whatever
        // else is open, unlike the fullScreen branch above.
        options |= ofTileable;
    }

    TRect r = getExtent();
    r.grow(-1, -1);
    grid_ = new TGridView(r, gridOptions);
    grid_->growMode = gfGrowHiX | gfGrowHiY;
    insert(grid_);
}
