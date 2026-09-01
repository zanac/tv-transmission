#pragma once
#include <string>

// UI language. English is the default; the numeric value (0, 1, ...)
// matches the order of entries in the TRadioButtons in
// SettingsDialog.cpp — keep them in sync if more are added.
enum class Language {
    English = 0,
    Italian = 1,
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
};
