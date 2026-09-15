#pragma once

#define Uses_TDialog
#define Uses_TButton
#include <tvision/tv.h>

#include <vector>
#include "../rpc/Tracker.h"
#include "../rpc/Peer.h"
#include "../rpc/TransmissionClient.h"
#include "../tvision-ext/TGridView.h"

// TButton with a persistently different look for "this one is the
// active tab" — used for the two tab buttons at the top of
// TrackerPeerWindow (see its own doc comment for why two buttons stand
// in for a genuine tab control tvision doesn't have). Reuses the SAME
// color pairing tvision's own TButton already uses for a keyboard-
// FOCUSED button (see drawState()'s own sfSelected branch in
// tbutton.cpp) rather than inventing a new color from scratch — matches
// the existing theme instead of clashing with it. TButton::getPalette()
// is virtual specifically so a subclass CAN override just this one
// piece; drawState() itself (not virtual, and so not overridden here)
// keeps doing all its own state-driven color-selection logic
// unchanged — it just resolves through a different palette string when
// active_ is true. Both tab buttons stay enabled/clickable regardless
// of which is active (unlike an earlier version of this that disabled
// the active one instead — see "Fixed bugs" in the README for why):
// clicking the one you're already on is a harmless no-op, and disabling
// it would have meant losing this class's own color override entirely,
// since TButton::drawState()'s disabled-state branch bypasses the
// normal/selected color logic this depends on altogether.
class TTabButton : public TButton {
public:
    TTabButton(const TRect& bounds, TStringView title, ushort command)
        : TButton(bounds, title, command, bfNormal) {}

    void setActive(bool active) {
        if (active == active_) return;
        active_ = active;
        drawView();
    }

    TPalette& getPalette() const override {
        // Index 1 substituted with index 3's own value: TButton::
        // drawState()'s "normal, not specially focused" branch calls
        // getColor(0x0501) (palette indices 5 and 1) — swapping index 1
        // to match what index 3 holds makes that resolve to the exact
        // same colors getColor(0x0703) (indices 7 and 3, the SELECTED-
        // button case) already would under the unmodified palette.
        // Index 5 doesn't need its own change: it already holds the
        // same value as index 7 in TButton's own default palette
        // (cpButton — see tbutton.cpp).
        static TPalette activePalette("\x0C\x0B\x0C\x0D\x0E\x0E\x0E\x0F", 8);
        return active_ ? activePalette : TButton::getPalette();
    }

    // TButton::draw() (== drawState(False), see tbutton.cpp — drawState
    // itself isn't virtual, so this is the only hook available) always
    // draws a one-column shadow along its own right edge, regardless of
    // what's next to it — with two tab buttons placed directly against
    // each other (no gap in their own bounds — see TrackerPeerWindow's
    // own constructor), that shadow column is exactly what still read
    // as a visible gap between them even once the bounds themselves
    // touched. Rather than reimplementing drawState()'s own fairly
    // involved rendering from scratch just to omit one column, this
    // draws normally via the base class first, then overwrites that one
    // column with a blank cell in this button's own current background
    // color — recomputed here the same way drawState() itself would
    // (disabled / selected / default / plain), so the patched-over
    // column always matches whatever the rest of the button just drew,
    // in whatever state it's actually in.
    // TButton::drawState() (see tbutton.cpp — not virtual, so this is
    // the only hook available, via the virtual draw() that just calls
    // it) shades BOTH edges of every button, not just the right one:
    // b.putAttribute(0, cShadow) runs unconditionally for column 0 (the
    // LEFT edge) on every row, alongside the right-edge shadow at
    // column size.x-1 already patched below. Missing the left edge
    // the first time this was fixed meant the SECOND tab button's own
    // left edge was still shaded even after the first button's own
    // right edge was patched — the seam between two adjacent tab
    // buttons is made of two different edges, one from each button, not
    // one. Both patched here the same way: draw normally via the base
    // class, then overwrite each shaded column with a blank cell in
    // this button's own current background color.
    void draw() override {
        TButton::draw();
        TAttrPair cButton;
        if ((state & sfDisabled) != 0) {
            cButton = getColor(0x0404);
        } else {
            cButton = getColor(0x0501);
            if ((state & sfActive) != 0) {
                if ((state & sfSelected) != 0) cButton = getColor(0x0703);
                else if (amDefault) cButton = getColor(0x0602);
            }
        }
        TDrawBuffer b;
        b.moveChar(0, ' ', cButton, 1);
        writeLine(0, 0, 1, 1, b);
        writeLine(size.x - 1, 0, 1, 1, b);
    }

private:
    bool active_ = false;
};

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
    TTabButton* trackersTabButton_ = nullptr;
    TTabButton* peersTabButton_ = nullptr;
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
