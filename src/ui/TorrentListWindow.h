#pragma once

#define Uses_TEvent
#include <tvision/tv.h>

#include <functional>
#include <vector>
#include "../AppSettings.h"
#include "../rpc/TransmissionClient.h"
#include "../rpc/Torrent.h"
#include "../tgridview/TGridWindow.h"

// Called whenever the user changes the sort column/direction (header
// click — the toggle-direction and indicator-drawing logic itself now
// lives in TGridView; see TGridView::SortChangedFn), so the caller can
// persist it (see App::newTorrentListWindow()).
using SortChangedCallback = std::function<void(SortColumn, bool)>;

// The main torrent list — a thin app-specific layer on top of the
// generic TGridView (src/tgridview/), which supplies the actual
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
    TorrentListWindow(const TRect& bounds, TransmissionClient& client,
                       SortColumn initialSort, bool initialAscending,
                       TorrentFilter initialFilter,
                       const std::vector<int>& initialColumnWidths,
                       const std::vector<int>& initialColumnOrder,
                       const std::vector<bool>& initialColumnVisible,
                       SortChangedCallback onSortChanged,
                       const std::vector<int>& initialTrackerColumnWidths = {},
                       const std::vector<int>& initialTrackerColumnOrder = {},
                       const std::vector<bool>& initialTrackerColumnVisible = {});

    void refresh();       // calls listTorrents() and updates the view
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

    TransmissionClient& client_;
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
};
