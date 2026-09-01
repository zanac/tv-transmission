#pragma once

#define Uses_TWindow
#define Uses_TInputLine
#define Uses_TCheckBoxes
#include <tvision/tv.h>
#include "../rpc/Torrent.h"
#include "../rpc/TransmissionClient.h"

// Plain TWindow (default palette — matching the Settings dialog, which
// the user liked as-is) with a couple of extras: it knows which torrent
// it's showing (so an already-open details window can be reused instead
// of duplicated) and handles its own "Apply"/"Close" buttons.
//
// Unlike the rest of this window's content (a snapshot taken when
// opened), the speed limit controls are live: "Apply" sends a
// torrent-set RPC call right away using torrentId_/client_.
class TorrentDetailsWindow : public TWindow {
public:
    TorrentDetailsWindow(const TRect& bounds, TStringView title, short number,
                          int torrentId, TransmissionClient& client);

    void handleEvent(TEvent& event) override;

    // So the caller (TorrentListWindow) can find an already-open details
    // window for a given torrent instead of opening a duplicate.
    int torrentId() const { return torrentId_; }

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
