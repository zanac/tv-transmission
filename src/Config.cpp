#include "Config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

using json = nlohmann::json;

namespace {

std::string configDirPath() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg)
        return std::string(xdg) + "/tv-transmission";
    const char* home = std::getenv("HOME");
    return std::string(home ? home : ".") + "/.config/tv-transmission";
}

// Minimal mkdir -p: creates one directory level at a time starting from
// the root, ignoring "already exists" errors.
bool ensureDirExists(const std::string& path) {
    for (size_t i = 1; i <= path.size(); ++i) {
        if (i == path.size() || path[i] == '/') {
            std::string cur = path.substr(0, i);
            if (!cur.empty()) mkdir(cur.c_str(), 0700);
        }
    }
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

} // namespace

std::string configFilePath() {
    return configDirPath() + "/settings.json";
}

AppSettings loadSettings() {
    AppSettings settings; // starts from AppSettings.h's defaults
    std::ifstream in(configFilePath());
    if (!in) return settings; // first run, no file yet: defaults are fine

    try {
        json j;
        in >> j;
        settings.refreshIntervalSeconds =
            j.value("refreshIntervalSeconds", settings.refreshIntervalSeconds);
        settings.host = j.value("host", settings.host);
        settings.port = j.value("port", settings.port);
        settings.user = j.value("user", settings.user);
        settings.password = j.value("password", settings.password);
        int lang = j.value("language", static_cast<int>(settings.language));
        settings.language = static_cast<Language>(lang);
    } catch (const std::exception&) {
        // Corrupted/malformed file: better to fall back to defaults than
        // to block the app from starting.
        return AppSettings{};
    }
    return settings;
}

bool saveSettings(const AppSettings& settings) {
    std::string dir = configDirPath();
    if (!ensureDirExists(dir)) return false;

    json j;
    j["refreshIntervalSeconds"] = settings.refreshIntervalSeconds;
    j["host"] = settings.host;
    j["port"] = settings.port;
    j["user"] = settings.user;
    j["password"] = settings.password;
    j["language"] = static_cast<int>(settings.language);

    std::string path = configFilePath();
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << j.dump(2);
    out.close();
    if (!out) return false;

    // Contains the RPC password in plain text: restrict permissions to
    // read/write for the current user only.
    chmod(path.c_str(), 0600);

    return true;
}
