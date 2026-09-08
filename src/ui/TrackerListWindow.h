#pragma once

#define Uses_TDialog
#include <tvision/tv.h>

#include <vector>
#include "../rpc/Tracker.h"
#include "../rpc/TransmissionClient.h"
#include "../tgridview/TGridView.h"

// Non-modal window listing every tracker for one torrent — host, tier,
// seeders, leechers, downloaded count, and a short status — built on
// TGridView (the same generic widget the main torrent list and the
// files window use), with a manual "Refresh" button (this data isn't
// part of the app's periodic refresh — see TransmissionClient::
// getTrackerStats()) and "Close". Column resizing/reordering/showing-
// hiding is available too, but through the app's single, focus-aware
// "Manage columns..." menu entry rather than a button of its own here —
// see App::focusedGrid()/showColumnManagerDialog().
// Double-clicking a row opens a small window with that tracker's full
// details (error message, last/next announce times), which don't fit
// in a table row.
//
// Columns aren't sortable here (unlike the main torrent list) —
// deliberately: the tracker count per torrent is small and Transmission
// already returns them in tier order, which is the order that matters,
// so letting a click reorder rows would work against that rather than
// help. Resizing/reordering/hiding the *columns* themselves is still
// offered, since that's a display preference independent of row order.
//
// TDialog rather than TWindow for the same reason as
// TorrentDetailsWindow (see the comment there): matches the rest of the
// app's default color palette.
class TrackerListWindow : public TDialog {
public:
    // `initialColumnWidths`/`initialColumnOrder`/`initialColumnVisible`:
    // same three-vector shape and fallback rules as TorrentListWindow's
    // own constructor — from AppSettings::trackerColumnWidths/Order/
    // Visible, so a previous session's (or another already-open tracker
    // window's) column choices carry over. Empty, or a mismatched
    // count, falls back to this window's own built-in defaults.
    TrackerListWindow(const TRect& bounds, TStringView title,
                       int torrentId, TransmissionClient& client,
                       const std::vector<int>& initialColumnWidths = {},
                       const std::vector<int>& initialColumnOrder = {},
                       const std::vector<bool>& initialColumnVisible = {});

    void handleEvent(TEvent& event) override;

    // So the caller (TorrentDetailsWindow) can find an already-open
    // tracker list for this torrent instead of opening a duplicate.
    int torrentId() const { return torrentId_; }

    // Current column widths/order/visibility, same conventions as the
    // constructor's own initial* parameters — read by App::
    // showColumnManagerDialog() to persist whatever was last changed
    // here into AppSettings::trackerColumnWidths/Order/Visible.
    std::vector<int> columnWidths() const;
    std::vector<int> columnOrder() const;
    std::vector<bool> columnVisibility() const;

private:
    void refresh();
    void showDetailForSelected();

    int torrentId_;
    TransmissionClient& client_;
    TGridView* grid_ = nullptr;
    std::vector<TrackerStat> trackers_;
};

// Creates the tracker list window for a given torrent (fetches the
// initial data immediately). `initialColumnWidths`/`initialColumnOrder`/
// `initialColumnVisible`: forwarded to TrackerListWindow's own
// constructor — see its doc comment.
TDialog* createTrackerListWindow(int torrentId, const std::string& torrentName,
                                  TransmissionClient& client,
                                  const std::vector<int>& initialColumnWidths = {},
                                  const std::vector<int>& initialColumnOrder = {},
                                  const std::vector<bool>& initialColumnVisible = {});
