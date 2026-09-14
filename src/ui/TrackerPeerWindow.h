#pragma once

#define Uses_TDialog
#define Uses_TButton
#include <tvision/tv.h>

#include <vector>
#include "../rpc/Tracker.h"
#include "../rpc/Peer.h"
#include "../rpc/TransmissionClient.h"
#include "../tvision-ext/TGridView.h"

// Non-modal window listing per-torrent live data, in one of two tabs —
// Trackers (host, tier, seeders, leechers, downloaded count, status) or
// Peers (address, client, progress, down/up speed, flags) — switched
// via two buttons at the top acting as tabs (see switchToTab()), built
// on TGridView (the same generic widget the main torrent list and the
// files window use). Neither tab is part of the app's periodic
// refresh (see TransmissionClient::getTrackerStats()/getPeers()) —
// each is fetched on demand, whenever it's the one actually showing,
// with a manual "Refresh" button and "Close". Column resizing/
// reordering/showing-hiding is available too, but through the app's
// single, focus-aware "Manage columns..." menu entry rather than a
// button of its own here — see App::focusedGrid()/
// showColumnManagerDialog(), and isPeersTabActive() below for how that
// knows which tab's own columns to save.
// Double-clicking a tracker row (only — peers have no equivalent) opens
// a small window with that tracker's full details (error message,
// last/next announce times), which don't fit in a table row.
//
// The Trackers tab's own columns aren't sortable (unlike the main
// torrent list) — deliberately: the tracker count per torrent is small
// and Transmission already returns them in tier order, which is the
// order that matters, so letting a click reorder rows would work
// against that rather than help. The Peers tab's columns aren't
// sortable either, for a related but different reason: the peer list
// itself reorders on every refresh regardless (Transmission doesn't
// promise a stable order the way it does for trackers), so a click-to-
// sort here would just as easily un-sort itself moments later.
// Resizing/reordering/hiding the *columns* themselves is still offered
// on both tabs, since that's a display preference independent of row
// order.
//
// TDialog rather than TWindow for the same reason as
// TorrentDetailsWindow (see the comment there): matches the rest of the
// app's default color palette.
class TrackerPeerWindow : public TDialog {
public:
    // `initialColumnWidths`/`initialColumnOrder`/`initialColumnVisible`
    // are for the Trackers tab; `initialPeerColumnWidths`/
    // `initialPeerColumnOrder`/`initialPeerColumnVisible` for the Peers
    // one — same three-vector shape and fallback rules either way, from
    // AppSettings::trackerColumnWidths/Order/Visible and
    // AppSettings::peerColumnWidths/Order/Visible respectively, so a
    // previous session's (or another already-open tracker window's)
    // column choices carry over for whichever tab is opened. Empty, or
    // a mismatched count, falls back to that tab's own built-in
    // defaults. The Trackers tab is what's shown first, regardless of
    // which one was active the last time any tracker window was open —
    // not itself persisted, unlike the columns.
    TrackerPeerWindow(const TRect& bounds, TStringView title,
                       int torrentId, TransmissionClient& client,
                       const std::vector<int>& initialColumnWidths = {},
                       const std::vector<int>& initialColumnOrder = {},
                       const std::vector<bool>& initialColumnVisible = {},
                       const std::vector<int>& initialPeerColumnWidths = {},
                       const std::vector<int>& initialPeerColumnOrder = {},
                       const std::vector<bool>& initialPeerColumnVisible = {});

    void handleEvent(TEvent& event) override;

    // So the caller (TorrentDetailsWindow) can find an already-open
    // tracker list for this torrent instead of opening a duplicate.
    int torrentId() const { return torrentId_; }

    // Identifies which server's TransmissionClient this window is
    // talking to, by address — see TorrentDetailsWindow::clientPtr()'s
    // own comment for what this is for.
    TransmissionClient* clientPtr() const { return &client_; }

    // Current column widths/order/visibility for whichever tab is
    // ACTIVE right now — same conventions as the constructor's own
    // initial* parameters — read by App::showColumnManagerDialog() to
    // persist whatever was last changed here into the right pair of
    // AppSettings fields (see isPeersTabActive() below for which pair).
    std::vector<int> columnWidths() const;
    std::vector<int> columnOrder() const;
    std::vector<bool> columnVisibility() const;

    // True if the Peers tab (not Trackers) is the one currently shown —
    // App::showColumnManagerDialog() and App::shutDown()'s own backstop
    // both need this to know whether columnWidths()/columnOrder()/
    // columnVisibility() above just reflected the Trackers or the Peers
    // tab, since the two are saved into separate AppSettings fields.
    bool isPeersTabActive() const { return activeTab_ == Tab::Peers; }

private:
    enum class Tab { Trackers, Peers };

    void refresh();
    void showDetailForSelected();
    // Rebuilds grid_'s own columns and row-count for `tab`, applying
    // whichever initial*/current column layout belongs to it, and
    // re-fetches that tab's own data — called once from the
    // constructor (for the Trackers tab, shown first — see the
    // constructor's own doc comment) and again on every tab-button
    // click afterward.
    void switchToTab(Tab tab);

    int torrentId_;
    TransmissionClient& client_;
    TGridView* grid_ = nullptr;
    TButton* trackersTabButton_ = nullptr;
    TButton* peersTabButton_ = nullptr;
    Tab activeTab_ = Tab::Trackers;
    std::vector<TrackerStat> trackers_;
    std::vector<Peer> peers_;
    // Remembers each tab's own column layout independently while
    // switching between them within the SAME window instance — without
    // this, switching to Peers and back would lose whatever widths/
    // order/visibility were set on Trackers in this session (grid_'s
    // own columns get entirely replaced on every switchToTab() call),
    // even though nothing was ever explicitly saved via "Manage
    // columns...". Seeded from the constructor's own initial*
    // parameters, updated from grid_ itself just before switching away
    // from a tab.
    std::vector<int> trackerColumnWidths_;
    std::vector<int> trackerColumnOrder_;
    std::vector<bool> trackerColumnVisible_;
    std::vector<int> peerColumnWidths_;
    std::vector<int> peerColumnOrder_;
    std::vector<bool> peerColumnVisible_;
};

// Creates the tracker/peers window for a given torrent (fetches the
// initial data immediately). `initialColumnWidths`/`initialColumnOrder`/
// `initialColumnVisible`/`initialPeerColumnWidths`/
// `initialPeerColumnOrder`/`initialPeerColumnVisible`: forwarded to
// TrackerPeerWindow's own constructor — see its doc comment.
TDialog* createTrackerPeerWindow(int torrentId, const std::string& torrentName,
                                  TransmissionClient& client,
                                  const std::vector<int>& initialColumnWidths = {},
                                  const std::vector<int>& initialColumnOrder = {},
                                  const std::vector<bool>& initialColumnVisible = {},
                                  const std::vector<int>& initialPeerColumnWidths = {},
                                  const std::vector<int>& initialPeerColumnOrder = {},
                                  const std::vector<bool>& initialPeerColumnVisible = {});
