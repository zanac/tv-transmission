#pragma once

#define Uses_TDialog
#include <tvision/tv.h>

#include <vector>
#include "../rpc/Torrent.h"
#include "../rpc/TransmissionClient.h"
#include "../tgridview/TGridView.h"

// Non-modal window listing every file within one torrent, grouped into
// the same folder structure the torrent's own file paths describe —
// name, size, download progress, whether it's wanted, and its priority
// — with controls to change wanted/priority for the focused row, file
// or folder alike (a folder applies the action to every real file
// beneath it, however deep). Built on TGridView for the list itself,
// the same generic widget the main torrent list uses.
//
// TDialog rather than TWindow for the same reason as
// TorrentDetailsWindow/TrackerListWindow: matches the rest of the app's
// default color palette.

// One row as actually displayed — a real file, or a synthetic folder
// row aggregating every file beneath it. `fileIndices` are indices into
// TorrentFilesWindow's own `files_` (the flat, RPC-order list Transmission
// itself addresses files by) — a single entry for a file row, every
// descendant's index for a folder row. Torrents with no folders at all
// (every file directly at the top level) produce exactly one row per
// file and no folder rows, identical to the flat list this replaced.
struct FileTreeRow {
    std::string name;   // just this row's own path segment, not the full path
    int depth = 0;       // indentation level, root's direct children are 0
    bool isFolder = false;
    std::vector<int> fileIndices;
};

class TorrentFilesWindow : public TDialog {
public:
    TorrentFilesWindow(const TRect& bounds, TStringView title,
                        int torrentId, TransmissionClient& client);

    void handleEvent(TEvent& event) override;

    // So the caller (TorrentListWindow) can find an already-open files
    // window for this torrent instead of opening a duplicate — same
    // pattern as TorrentDetailsWindow::torrentId()/
    // TrackerListWindow::torrentId().
    int torrentId() const { return torrentId_; }

private:
    void refresh();
    void toggleWantedForFocused();
    void setPriorityForFocused(int priority);
    void setAllWanted(bool wanted);

    int torrentId_;
    TransmissionClient& client_;
    TGridView* grid_ = nullptr;
    std::vector<TorrentFile> files_; // flat, RPC order — real file indices for torrent-set calls
    std::vector<FileTreeRow> rows_;  // files_ regrouped into a folder tree — what's actually displayed
};

// Creates the files window for a given torrent (fetches the initial
// file list immediately).
TDialog* createTorrentFilesWindow(int torrentId, const std::string& torrentName,
                                   TransmissionClient& client);
