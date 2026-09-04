#pragma once
#include <string>

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
enum class SortColumn { Name = 0, Done = 1, Size = 2, Down = 3, Up = 4, Added = 5, Status = 6 };

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

// Settings the user can configure from the Settings window.
struct AppSettings {
    int refreshIntervalSeconds = 5; // first option in the settings window
    std::string host = "127.0.0.1";
    int port = 9091;
    std::string user;
    std::string password;
    Language language = Language::English;

    // Last column/direction the torrent list was sorted by, so it's
    // restored on the next launch instead of always starting at Name/asc.
    SortColumn sortColumn = SortColumn::Name;
    bool sortAscending = true;

    // Persisted the same way as everything else here: saved when the
    // Filters window is confirmed, reloaded on the next launch.
    TorrentFilter filter;
};
