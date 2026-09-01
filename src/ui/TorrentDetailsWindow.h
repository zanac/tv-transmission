#pragma once

#define Uses_TWindow
#define Uses_TInputLine
#define Uses_TCheckBoxes
#include <tvision/tv.h>
#include "../rpc/Torrent.h"
#include "../rpc/TransmissionClient.h"

// TWindow with fixed colors (yellow text on black), independent of the
// app's overall theme. Overriding mapColor() here means every child
// view (the TStaticText labels) picks it up too: their own color
// requests eventually bubble up to this window's mapColor() through the
// owner chain (see TorrentListViewer::mapColor() in TorrentListWindow.h
// for the same technique, used there for the main list).
//
// Unlike the rest of this window's content (a snapshot taken when
// opened), the speed limit controls are live: "Apply" sends a
// torrent-set RPC call right away using torrentId_/client_.
class TorrentDetailsWindow : public TWindow {
public:
    TorrentDetailsWindow(const TRect& bounds, TStringView title, short number,
                          int torrentId, TransmissionClient& client);

    TColorAttr mapColor(uchar index) override;
    void handleEvent(TEvent& event) override;

    // Set by createTorrentDetailsWindow() once the controls are built.
    TCheckBoxes* limitCheckboxes = nullptr;
    TInputLine* downloadLimitField = nullptr;
    TInputLine* uploadLimitField = nullptr;

private:
    void applySpeedLimits();

    int torrentId_;
    TransmissionClient& client_;
};

// Creates a window with the main information about a torrent (a
// snapshot taken when opened, it doesn't refresh itself) plus controls
// to set or clear a per-torrent download/upload speed limit override
// (applied immediately via `client` when confirmed).
TWindow* createTorrentDetailsWindow(const Torrent& t, TransmissionClient& client);
