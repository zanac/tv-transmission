#pragma once

#define Uses_TEvent
#include <tvision/tv.h>

#include <functional>
#include <vector>
#include "../AppSettings.h"
#include "../rpc/TransmissionClient.h"
#include "../rpc/Torrent.h"
#include "../tvision-ext/TGridWindow.h"
#include "StatusPanel.h"
#include "FilesPanel.h"

// Called whenever the user changes the sort column/direction (header
// click — the toggle-direction and indicator-drawing logic itself now
// lives in TGridView; see TGridView::SortChangedFn), so the caller can
// persist it (see App::newTorrentListWindow()).
using SortChangedCallback = std::function<void(SortColumn, bool)>;
// Called on every keystroke/checkbox toggle in the Status panel (see
// setStatusPanelOpen()) — NOT this window's own setFilter(): `filter`
// is a single GLOBAL setting shared across every server's own window
// (see AppSettings::filter's own comment), the same way onSortChanged
// above hands cross-window coordination back to App rather than this
// window trying to reach every other one itself. App's own
// implementation both persists the new filter and applies it to every
// currently open window, including this one.
using FilterChangedCallback = std::function<void(const TorrentFilter&)>;

// The main torrent list — a thin app-specific layer on top of the
// generic TGridView (src/tvision-ext/), which supplies the actual
// column/row rendering, sorting-on-click UI, resizing, the scrollbar,
// and mouse handling. What lives here is everything genuinely
// torrent-specific: which columns exist and how wide, how a Torrent
// becomes each column's text, the status-based row coloring, and the
// actions (start/stop/remove/...) wired to the grid's callbacks. See
// "Fixed bugs" in README.md for the story of this migration.
class TorrentListWindow : public TGridWindow {
public:
    // `initialColumnWidths`: one width per column, same order as
    // SortColumn (Name, Done, Size, Down, Up, Added, Status) — from
    // AppSettings::columnWidths, so a previous session's resizing
    // survives a restart. Empty (or a mismatched count) falls back to
    // this window's own defaults, which also covers the very first run.
    //
    // `initialColumnOrder`: a permutation of [0, 7) — one entry per
    // visual position, each holding the LOGICAL column index (again
    // SortColumn's own order) shown there — from AppSettings::
    // columnOrder, so a previous session's rearranging survives a
    // restart too. Empty (or not a valid permutation) falls back to
    // identity order (Name, Done, Size, Down, Up, Added, Status, left to
    // right) — see TGridView::setColumnOrder()'s own doc comment for
    // exactly what "not valid" covers.
    // `initialColumnVisible`: one bool per column, same order as
    // SortColumn — from AppSettings::columnVisible, so a previous
    // session's column choices survive a restart. Empty (or a
    // mismatched count) falls back to every column shown.
    // `initialTrackerColumnWidths`/`Order`/`Visible`: forwarded, in
    // turn, to every TorrentDetailsWindow this opens (via
    // showDetailsForSelected()) — from AppSettings::
    // trackerColumnWidths/Order/Visible, so a tracker window opened
    // from any of them starts with whatever tracker column layout was
    // last saved.
    // `serverName`: this window's own logical server name (see
    // AppSettings::servers) — shown in the title, alongside the
    // translated "Torrents" text (see retranslate()), and read back by
    // App to know which settings.json entry this window's own geometry
    // belongs to when saving on exit.
    TorrentListWindow(const TRect& bounds, const std::string& serverName, TransmissionClient& client,
                       SortColumn initialSort, bool initialAscending,
                       TorrentFilter initialFilter,
                       const std::vector<int>& initialColumnWidths,
                       const std::vector<int>& initialColumnOrder,
                       const std::vector<bool>& initialColumnVisible,
                       SortChangedCallback onSortChanged,
                       FilterChangedCallback onFilterChanged,
                       const std::vector<int>& initialTrackerColumnWidths = {},
                       const std::vector<int>& initialTrackerColumnOrder = {},
                       const std::vector<bool>& initialTrackerColumnVisible = {},
                       const std::vector<int>& initialPeerColumnWidths = {},
                       const std::vector<int>& initialPeerColumnOrder = {},
                       const std::vector<bool>& initialPeerColumnVisible = {});

    const std::string& serverName() const { return serverName_; }

    // Window -> Panels -> Status: opens/closes the live filter panel on
    // the left, resizing the grid to make room (or give the space
    // back) — see relayoutPanels() in the .cpp. `width` is only used
    // when opening; ignored (the panel keeps whatever width it already
    // has) when closing, and irrelevant when neither.
    void setStatusPanelOpen(bool open, int width);
    bool isStatusPanelOpen() const { return statusPanel_ != nullptr; }
    // Current width — read by App::shutDown() to persist whatever the
    // user last dragged the panel's own border to (see AppSettings::
    // PanelLayout::statusWidth), the same way columnWidths() below is
    // read for the grid's own columns.
    int statusPanelWidth() const { return statusPanelWidth_; }

    // Window -> Panels -> Files: same idea as the Status panel above,
    // mirrored on the right instead of the left — see
    // relayoutPanels()'s own comment on why both share one
    // implementation there rather than each having its own.
    void setFilesPanelOpen(bool open, int width);
    bool isFilesPanelOpen() const { return filesPanel_ != nullptr; }
    int filesPanelWidth() const { return filesPanelWidth_; }

    // Keyboard-driven equivalent of dragResizeStatusPanel()/
    // dragResizeFilesPanel() (private, below — reached only via an
    // in-progress mouse drag already being handled in handleEvent()):
    // these two are public, since they're entered fresh each time from
    // outside this class entirely (App's own Panels-menu command
    // handlers, and this class's own handleEvent() reacting to
    // Ctrl+Left/Right whenever the matching panel currently has
    // keyboard focus) rather than continuing an event already being
    // processed. Left/Right resizes live, Enter confirms at the
    // current width, Esc cancels back to whatever width the panel had
    // on entry — same convention as TGridView::startKeyboardResize()
    // uses for a column's own width.
    void keyboardResizeStatusPanel();
    void keyboardResizeFilesPanel();

    void refresh();       // calls listTorrents() and updates the view

    // Non-blocking equivalent of refresh(), for App::idle()'s own
    // periodic refresh loop specifically — see TransmissionClient::
    // startRefresh()'s own comment for why. Starts the request on
    // `multi` (App's own shared CURLM* — see its own comment on why
    // there's exactly one); finishAsyncRefresh() applies the result
    // once App's own curl_multi_info_read() loop reports it done,
    // identified via this window's own `this` pointer (passed here as
    // curl's own CURLOPT_PRIVATE, read back via CURLINFO_PRIVATE).
    void startAsyncRefresh(CURLM* multi) { client_.startRefresh(multi, this); }
    bool isAsyncRefreshInFlight() const { return client_.isRefreshInFlight(); }
    // Never called except right after App's own curl_multi_info_read()
    // loop reports this window's own request as one of the ones that
    // just finished — never speculatively, and never twice for the
    // same startAsyncRefresh() call.
    void finishAsyncRefresh(CURLM* multi);
    // Re-syncs the shared Torrent-menu command enable/disable state
    // (see updateCommandStates()) to THIS window's own current
    // selection whenever it becomes the active one — enableCommand()/
    // disableCommand() are process-wide, not per-window, so without
    // this a window that gains focus without also changing its own row
    // focus (its usual trigger — see the grid's row-focus callback in
    // the constructor) would silently keep showing whichever other
    // window's state was set last, not its own.
    void setState(ushort aState, Boolean enable) override;
    // Overridden for exactly one reason: relayoutPanels() has to run
    // again on every resize this window gets from ANYTHING (the
    // terminal itself resizing, which is what actually reaches this in
    // practice given fullScreen's own flags=0 rules out interactive
    // resize — see TGridWindow.cpp's own comment on why), not only when
    // a panel is opened/closed/dragged — otherwise the grid and any
    // open panel would keep whatever bounds they had from BEFORE a
    // terminal resize, no longer matching this window's own new size.
    void changeBounds(const TRect& bounds) override;
    // Overridden for exactly one reason: catching a mouse click/drag
    // on the boundary between an open panel and the grid (there's
    // nothing else here that needs its own handleEvent) — see
    // relayoutPanels()'s own comment on why that boundary isn't a
    // separate view of its own with its own event handling.
    void handleEvent(TEvent& event) override;
    // Draws a visible vertical divider at each open panel's own
    // boundary with the grid, on top of whatever TGridWindow's own
    // draw() already drew — asked for directly, doubling as a visible
    // affordance for the same drag-to-resize/double-click-to-reset
    // boundary handleEvent() above already reacts to (previously an
    // invisible one-column gap the mouse simply happened to react to,
    // with nothing shown there to suggest it).
    void draw() override;
    void startSelected();
    void stopSelected();
    void removeSelected();       // confirmation prompt, then keeps files on disk
    void deleteWithDataSelected(); // confirmation prompt, then deletes files too
    void startNowSelected();
    void verifySelected();
    void reannounceSelected();
    void queueMoveTopForSelected();
    void queueMoveUpForSelected();
    void queueMoveDownForSelected();
    void queueMoveBottomForSelected();
    // Cycles through the four queue-move actions above, one per
    // double-click on the queue position column — see
    // cycleQueueActionForRow()'s own comment for why this is a single
    // shared counter rather than per-row state.
    void cycleQueueActionForRow(int row);
    // Sets the bandwidth priority (-1/0/1, see TransmissionClient::
    // setPriority()) for every currently selected torrent, or just the
    // focused one outside selection mode — same targetTorrents()-based
    // pattern as queueMoveTopForSelected() and the rest above.
    void setPriorityForSelected(int priority);
    // Cycles Low -> Normal -> High -> Low, one step per double-click on
    // the Priority column — unlike cycleQueueActionForRow()'s own
    // shared counter, this reads the ROW'S OWN current priority
    // directly (Torrent::bandwidthPriority already tells you where in
    // the cycle it is, which queue position's own four relative-move
    // actions have no equivalent of), so there's no separate counter
    // to keep in sync with anything.
    void cyclePriorityForRow(int row);
    void showDetailsForSelected();
    void showFilesForSelected();
    void retranslate();   // re-applies the title + column headers in the current language

    void setFilter(TorrentFilter filter); // applied to already-fetched data, no re-fetch
    const TorrentFilter& filter() const { return filter_; }

    // Current width of every column, same order as the constructor's
    // `initialColumnWidths` — read by App::shutDown() to persist
    // whatever the user last resized them to.
    std::vector<int> columnWidths() const;

    // Current visual arrangement, same convention as the constructor's
    // `initialColumnOrder` — read by App::shutDown() to persist whatever
    // the user last rearranged it to.
    std::vector<int> columnOrder() const;

    // Which columns are currently shown, same convention as the
    // constructor's `initialColumnVisible`.
    std::vector<bool> columnVisibility() const;
    // Applied immediately, e.g. right after the Columns dialog is
    // confirmed (see App::showColumnsDialog()).
    void setColumnVisibility(const std::vector<bool>& visible);

    double totalDownloadRate() const; // sum over VISIBLE (filtered) torrents
    double totalUploadRate() const;

private:
    void setupColumns(const std::vector<int>& initialWidths);
    void applyColumnLabels();   // (re)applies translated header text for the current language
    void applyFilterAndSort();  // rebuilds visible_ from allTorrents_ (filter, then sort)
    void updateCommandStates(); // enables/disables App.h's torrent commands for the focused row
    const Torrent* selectedTorrent() const;
    // The torrents an action from the Torrent menu should apply to:
    // every checked row, in order, if the grid is in selection mode
    // (see TGridView::isInSelectionMode()) and at least one is checked;
    // otherwise a single-element vector holding selectedTorrent() (or
    // empty if nothing's focused) — the existing one-at-a-time
    // behavior, unchanged. Every *Selected() action method below reads
    // its targets from this, so multi-select support only had to be
    // added here rather than separately in each one.
    std::vector<const Torrent*> targetTorrents() const;
    void showContextMenuFor(int row, TPoint screenPos);
    // Recomputes grid_'s own bounds (and statusPanel_'s, if open) from
    // this window's own CURRENT extent and statusPanelWidth_ — called
    // after every change to any of those three things (panel opened/
    // closed, its own width dragged, or this window itself resized —
    // see changeBounds() above), rather than each of those updating
    // bounds independently and risking drifting out of sync with each
    // other.
    void relayoutPanels();
    // kBorderX is the boundary's OWN X, in this window's local
    // coordinates — recomputed by relayoutPanels() and cached here
    // rather than re-derived on every mouse event, since handleEvent()
    // needs it on every evMouseDown to know whether a click landed on
    // the boundary at all.
    int statusPanelBorderX_ = -1; // -1: no panel open, no boundary to hit-test
    int filesPanelBorderX_ = -1;  // same idea, mirrored on the right
    static constexpr int kStatusPanelMinWidth = 12;
    static constexpr int kStatusPanelMaxWidth = 60;
    static constexpr int kStatusPanelDefaultWidth = 24;
    static constexpr int kFilesPanelMinWidth = 16;
    static constexpr int kFilesPanelMaxWidth = 70;
    static constexpr int kFilesPanelDefaultWidth = 30;
    void dragResizeStatusPanel(TEvent& event);
    void dragResizeFilesPanel(TEvent& event);

    TransmissionClient& client_;
    std::string serverName_;
    // True from the moment a refresh attempt (sync or async alike)
    // fails until the next one succeeds — reflected in the title bar
    // itself (see buildWindowTitle()'s own comment on why there, not a
    // popup) via updateTitleForConnectionState() below, called from
    // both refresh() and finishAsyncRefresh() after every attempt,
    // whether it succeeded or not.
    bool connectionLost_ = false;
    // Rebuilds the title only when connectionLost_ actually CHANGES —
    // called after every refresh attempt regardless, but a no-op most
    // of the time (most refreshes succeed, and most failures aren't
    // the first one in a row), so this doesn't mean reallocating the
    // title on every single tick.
    void updateTitleForConnectionState(bool lost);
    std::vector<int> initialTrackerColumnWidths_;
    // Which queue-move action a double-click on the queue column does
    // next — 0=top, 1=up, 2=down, 3=bottom, advancing (wrapping) after
    // every such double-click regardless of which row it landed on;
    // not tied to any one torrent's own state, since queue position is
    // relative to every other torrent, not something with its own
    // fixed "next value" the way a file's priority has.
    int queueActionCycle_ = 0;
    std::vector<int> initialTrackerColumnOrder_;
    std::vector<bool> initialTrackerColumnVisible_;
    std::vector<int> initialPeerColumnWidths_;
    std::vector<int> initialPeerColumnOrder_;
    std::vector<bool> initialPeerColumnVisible_;

    // allTorrents_ is every torrent listTorrents() last returned, in
    // server order; visible_ is the filtered-then-sorted subset actually
    // shown (what the grid's callbacks read from). Kept separate rather
    // than filtering allTorrents_ in place so changing the filter
    // (setFilter()) doesn't need a fresh RPC round-trip to reapply.
    std::vector<Torrent> allTorrents_;
    std::vector<Torrent> visible_;
    TorrentFilter filter_;
    SortColumn sortColumn_;
    bool sortAscending_;
    SortChangedCallback onSortChanged_;
    FilterChangedCallback onFilterChanged_;
    // Non-null exactly when the panel is open — see setStatusPanelOpen().
    StatusPanel* statusPanel_ = nullptr;
    int statusPanelWidth_ = kStatusPanelDefaultWidth;
    FilesPanel* filesPanel_ = nullptr;
    int filesPanelWidth_ = kFilesPanelDefaultWidth;
};
