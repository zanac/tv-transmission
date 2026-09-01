#pragma once
#include <string>

// UI language. English is the default; the numeric value (0, 1, ...)
// matches the order of entries in the TRadioButtons in
// SettingsDialog.cpp — keep them in sync if more are added.
enum class Language {
    English = 0,
    Italian = 1,
};

// Settings the user can configure from the Settings window.
struct AppSettings {
    int refreshIntervalSeconds = 5; // first option in the settings window
    std::string host = "127.0.0.1";
    int port = 9091;
    std::string user;
    std::string password;
    Language language = Language::English;
};
