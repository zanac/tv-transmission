#pragma once
#include <string>
#include <vector>
#include <map>

// UI language. English is the default; the numeric value (0, 1, ...)
// matches the order of entries in the language combo box in
// SettingsDialog.cpp — keep them in sync if more are added.
enum class Language {
    English = 0,
    Italian = 1,
    French = 2,
    German = 3,
    Spanish = 4,
};

// Column used to sort the torrent list; the numeric value matches the
// column order in each row (see kNameW/kDoneW/etc. and buildHeaderText()
// in TorrentListWindow.cpp) — also used to work out which header column
// was clicked, and persisted so the chosen sort survives a restart.
//
// Lives here (rather than in ui/TorrentListWindow.h, where it used to
// be) because AppSettings needs it and ui/ headers include this one,
// not the other way around.
enum class SortColumn {
    Name = 0, Done = 1, Size = 2, Down = 3, Up = 4, Added = 5, Status = 6,
    // Hidden-by-default columns (see TorrentListWindow::setupColumns())
    // — appended after the original 7 rather than interleaved, so a
    // settings.json from before they existed still parses its
    // sortColumn field to the same original meaning.
    Ratio = 7, Uploaded = 8, Downloaded = 9, Location = 10, Eta = 11,
    Peers = 12, QueuePosition = 13, Priority = 14, CompletedDate = 15,
};

// Total number of torrent-list columns (the values SortColumn takes,
// 0..kTorrentColumnCount-1) — shared by TorrentListWindow and
// ColumnManagerDialog instead of each hardcoding the count separately.
constexpr int kTorrentColumnCount = 16;

// Which torrents show up in the list. A torrent must satisfy ALL of
// these to be shown (AND, not OR) — an empty/all-true filter (see
// isDefault()) shows every torrent, same as if filtering didn't exist.
struct TorrentFilter {
    std::string nameContains; // case-insensitive substring match; empty = no name filter

    // One flag per Transmission torrent status (tr_torrent_activity: 0
    // stopped .. 6 seeding — see TorrentListWindow.cpp's isStopped()/
    // isQueued() for the same enumeration used elsewhere). All true by
    // default: no status filtering, same as unchecking nothing in the
    // Filters window.
    bool showStopped = true;
    bool showCheckWait = true;
    bool showChecking = true;
    bool showDownloadWait = true;
    bool showDownloading = true;
    bool showSeedWait = true;
    bool showSeeding = true;

    bool isDefault() const {
        return nameContains.empty() && showStopped && showCheckWait && showChecking &&
               showDownloadWait && showDownloading && showSeedWait && showSeeding;
    }
};

// One server's own connection details, under whatever logical name the
// user gave it in the Connection dialog's server combo (see
// AppSettings::servers below) — e.g. "home", "seedbox". First step
// toward managing more than one server: for now, exactly one of these
// is ever actually connected to at a time (see AppSettings::
// activeServer) — a later step is expected to let more than one be
// active simultaneously.
struct ServerProfile {
    std::string host = "127.0.0.1";
    int port = 9091;
    std::string user;
    std::string password;
};

// Settings the user can configure from the Connection/Server windows.
struct AppSettings {
    int refreshIntervalSeconds = 5; // first option in the Connection window

    // Every server the user has ever named via the Connection dialog's
    // server combo, keyed by that logical name — added/removed there
    // via its own "[+]"/"[-]" buttons, not from a separate management
    // screen. A std::map (not unordered_map) so iterating it — e.g. to
    // rebuild the combo's own list on the next launch — comes out in a
    // stable, alphabetical order rather than an arbitrary one.
    //
    // Replaces this struct's own former flat host/port/user/password
    // fields directly — an older settings.json that still has those
    // (rather than this "servers" object) simply won't populate
    // `servers` at all when loaded (see Config.cpp's own loadSettings())
    // rather than being migrated into a single entry here: this project
    // is early enough that starting over instead of translating the old
    // shape was the simpler, deliberate choice.
    std::map<std::string, ServerProfile> servers;
    // Which entry in `servers` is the one actually connected to right
    // now — empty if none has been configured yet (a fresh install, or
    // exactly the "recreate rather than migrate" case above).
    std::string activeServer;

    Language language = Language::English;

    // Last column/direction the torrent list was sorted by, so it's
    // restored on the next launch instead of always starting at Name/asc.
    SortColumn sortColumn = SortColumn::Name;
    bool sortAscending = true;

    // Persisted the same way as everything else here: saved when the
    // Filters window is confirmed, reloaded on the next launch.
    TorrentFilter filter;

    // Current width of each torrent-list column, same order as
    // SortColumn (Name, Done, Size, Down, Up, Added, Status) — read
    // from the grid and saved on exit (see App::shutDown()), applied
    // back when the list is built on the next launch. Empty (or a
    // mismatched count, e.g. a settings.json from before a column was
    // added) falls back to the list's own built-in defaults.
    std::vector<int> columnWidths;

    // Current visual arrangement of the torrent-list columns — a
    // permutation of [0, 7), one entry per visual position holding the
    // LOGICAL column index (SortColumn's own order) shown there. Saved
    // and restored the same way as columnWidths above. Empty (or not a
    // valid permutation — see TGridView::setColumnOrder()) falls back
    // to identity order.
    std::vector<int> columnOrder;

    // Which torrent-list columns are shown at all, same order as
    // SortColumn — chosen from the "Columns..." dialog (Settings menu).
    // Saved as soon as that dialog is confirmed (unlike widths/order,
    // which are only captured on exit — see App::shutDown() — this is a
    // discrete dialog choice, not a continuous drag). Empty (or a
    // mismatched count) falls back to every column shown.
    std::vector<bool> columnVisible;

    // Same three fields as columnWidths/columnOrder/columnVisible
    // above, but for TrackerListWindow's own 6 columns (Host, Tier,
    // Seeders, Leechers, Downloaded, Status) instead of the main
    // torrent list's. Shared across every open tracker window — there's
    // one tracker column layout, not one per torrent — saved whenever
    // "Manage columns..." (see App::focusedGrid()) is used while a
    // tracker window has focus, and applied to every tracker window
    // opened afterward, including ones for a different torrent.
    std::vector<int> trackerColumnWidths;
    std::vector<int> trackerColumnOrder;
    std::vector<bool> trackerColumnVisible;

    // The currently active server's own connection details, or a
    // default-constructed ServerProfile if activeServer is empty or
    // doesn't match anything in servers — callers that just want "the
    // connection settings to use right now" (App's own constructor, the
    // CLI) don't each need to handle "nothing configured yet" as a
    // special case themselves.
    const ServerProfile& activeProfile() const {
        static const ServerProfile empty;
        auto it = servers.find(activeServer);
        return it != servers.end() ? it->second : empty;
    }
};
