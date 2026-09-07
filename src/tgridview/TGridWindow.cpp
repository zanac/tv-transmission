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
        flags = 0;
        options |= ofTileable;
    }

    TRect r = getExtent();
    r.grow(-1, -1);
    grid_ = new TGridView(r, gridOptions);
    grid_->growMode = gfGrowHiX | gfGrowHiY;
    insert(grid_);
}
