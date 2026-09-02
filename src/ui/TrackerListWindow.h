#pragma once

#define Uses_TDialog
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TEvent
#include <tvision/tv.h>

#include <vector>
#include "../rpc/Tracker.h"
#include "../rpc/TransmissionClient.h"

// TListViewer that renders one row per tracker: host, tier, seeders,
// leechers, downloaded count, short status. No sorting (unlike the main
// torrent list) — the tracker count per torrent is small and Transmission
// already returns them in tier order, which is the order that matters.
class TrackerListViewer : public TListViewer {
public:
    TrackerListViewer(const TRect& r, TScrollBar* vScrollBar);

    void setTrackers(std::vector<TrackerStat> trackers);
    const TrackerStat* selectedTracker() const;

    void getText(char* dest, short item, short maxLen) override;

private:
    std::vector<TrackerStat> trackers_;
};

// Non-modal window listing every tracker for one torrent, with a manual
// "Refresh" button (this data isn't part of the app's periodic refresh —
// see TransmissionClient::getTrackerStats()) and a "Close" button.
// Double-clicking a row opens a small window with that tracker's full
// details (error message, last/next announce times), which don't fit
// in a table row.
//
// TDialog rather than TWindow for the same reason as
// TorrentDetailsWindow (see the comment there): matches the rest of the
// app's default color palette.
class TrackerListWindow : public TDialog {
public:
    TrackerListWindow(const TRect& bounds, TStringView title,
                       int torrentId, TransmissionClient& client);

    void handleEvent(TEvent& event) override;

private:
    void refresh();
    void showDetailForSelected();

    int torrentId_;
    TransmissionClient& client_;
    TrackerListViewer* listViewer_ = nullptr;
};

// Creates the tracker list window for a given torrent (fetches the
// initial data immediately).
TDialog* createTrackerListWindow(int torrentId, const std::string& torrentName,
                                  TransmissionClient& client);
