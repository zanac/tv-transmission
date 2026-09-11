#include "TGridWindow.h"

TGridWindow::TGridWindow(const TRect& bounds, TStringView title, bool fullScreen,
                          ushort gridOptions, bool closable)
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
        // this branch exists for. `closable` is ignored here for the
        // same reason: flags = 0 already covers it.
        flags = 0;
        // Still needs to track the OWNER's size (normally TDeskTop) on
        // a terminal resize — flags=0 only strips the interactive
        // move/resize gestures, it says nothing about how this view
        // reacts when its owner's own bounds change. Without this, a
        // fullScreen window stays exactly the pixel size it was
        // created at forever, silently leaving gaps (or clipping)
        // after the terminal is resized, even though the user has no
        // way to fix it by hand anymore (that's the whole point of
        // flags=0). gfGrowHiX|gfGrowHiY keeps the top-left corner
        // fixed and grows/shrinks the bottom-right one to match —
        // exactly "always the full desktop", kept true across resizes
        // rather than just true at creation time. Same pairing
        // grid_->growMode already uses below, one level up.
        growMode = gfGrowHiX | gfGrowHiY;
    } else {
        // The MDI case: free to be tiled/cascaded alongside whatever
        // else is open, unlike the fullScreen branch above. wfClose —
        // otherwise part of TWindow's own default flags — is stripped
        // back out when the caller doesn't want this one closable
        // (see this constructor's own doc comment for why that's a
        // real, separate need from fullScreen): the same
        // dangling-pointer hazard the fullScreen branch avoids above
        // applies here too, for any caller that (like this app's own
        // TorrentListWindow) holds onto one of these across turns
        // rather than only ever looking it up fresh.
        options |= ofTileable;
        if (!closable) flags &= ~wfClose;
    }

    TRect r = getExtent();
    r.grow(-1, -1);
    grid_ = new TGridView(r, gridOptions);
    grid_->growMode = gfGrowHiX | gfGrowHiY;
    insert(grid_);
}
