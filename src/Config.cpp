#include "Config.h"
#include "Obfuscation.h"
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
        settings.password = deobfuscatePassword(j.value("password", std::string()));
        int lang = j.value("language", static_cast<int>(settings.language));
        settings.language = static_cast<Language>(lang);
        int sortCol = j.value("sortColumn", static_cast<int>(settings.sortColumn));
        settings.sortColumn = static_cast<SortColumn>(sortCol);
        settings.sortAscending = j.value("sortAscending", settings.sortAscending);

        if (j.contains("filter") && j["filter"].is_object()) {
            auto& f = j["filter"];
            settings.filter.nameContains = f.value("nameContains", settings.filter.nameContains);
            settings.filter.showStopped = f.value("showStopped", settings.filter.showStopped);
            settings.filter.showCheckWait = f.value("showCheckWait", settings.filter.showCheckWait);
            settings.filter.showChecking = f.value("showChecking", settings.filter.showChecking);
            settings.filter.showDownloadWait = f.value("showDownloadWait", settings.filter.showDownloadWait);
            settings.filter.showDownloading = f.value("showDownloading", settings.filter.showDownloading);
            settings.filter.showSeedWait = f.value("showSeedWait", settings.filter.showSeedWait);
            settings.filter.showSeeding = f.value("showSeeding", settings.filter.showSeeding);
        }
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
    j["password"] = obfuscatePassword(settings.password);
    j["language"] = static_cast<int>(settings.language);
    j["sortColumn"] = static_cast<int>(settings.sortColumn);
    j["sortAscending"] = settings.sortAscending;
    j["filter"] = {
        {"nameContains", settings.filter.nameContains},
        {"showStopped", settings.filter.showStopped},
        {"showCheckWait", settings.filter.showCheckWait},
        {"showChecking", settings.filter.showChecking},
        {"showDownloadWait", settings.filter.showDownloadWait},
        {"showDownloading", settings.filter.showDownloading},
        {"showSeedWait", settings.filter.showSeedWait},
        {"showSeeding", settings.filter.showSeeding},
    };

    std::string path = configFilePath();
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << j.dump(2);
    out.close();
    if (!out) return false;

    // The RPC password is obfuscated (see Obfuscation.h) rather than
    // stored in plain text, but that's not real encryption — 0600
    // permissions (only the current user can read the file) remain the
    // actual protection here, so keep them regardless.
    chmod(path.c_str(), 0600);

    return true;
}
