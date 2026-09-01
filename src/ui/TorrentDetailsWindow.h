#pragma once

#define Uses_TWindow
#include <tvision/tv.h>
#include "../rpc/Torrent.h"

// TWindow with fixed colors (yellow text on black), independent of the
// app's overall theme. Overriding mapColor() here means every child
// view (the TStaticText labels) picks it up too: their own color
// requests eventually bubble up to this window's mapColor() through the
// owner chain (see TorrentListViewer::mapColor() in TorrentListWindow.h
// for the same technique, used there for the main list).
class TorrentDetailsWindow : public TWindow {
public:
    TorrentDetailsWindow(const TRect& bounds, TStringView title, short number);

    TColorAttr mapColor(uchar index) override;
};

// Creates a window with the main information about a torrent (a
// snapshot taken when opened, it doesn't refresh itself).
TWindow* createTorrentDetailsWindow(const Torrent& t);

