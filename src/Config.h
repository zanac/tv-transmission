#pragma once
#include <string>
#include "AppSettings.h"

// Path of the config file:
// $XDG_CONFIG_HOME/tv-transmission/settings.json, or
// ~/.config/tv-transmission/settings.json if XDG_CONFIG_HOME isn't set.
std::string configFilePath();

// Loads settings from the config file. If the file doesn't exist or is
// unreadable/malformed, returns AppSettings{} (the defaults) without
// blocking the app's startup.
AppSettings loadSettings();

// Saves settings to the config file (creating the directory if needed).
// WARNING: the file contains the RPC password in plain text (no
// encryption) — it's written with 0600 permissions (only the current
// user can read it), but it's still plain text on disk. Returns false if
// saving fails (directory not creatable, permissions, etc.); the app
// keeps running either way.
bool saveSettings(const AppSettings& settings);
