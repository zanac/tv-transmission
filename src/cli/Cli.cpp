#include "Cli.h"
#include "../rpc/TransmissionClient.h"
#include "../ui/Strings.h"
#include "../TextUtil.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

void printUsage() {
    std::printf("%s\n", tr(Str::CliUsage));
}

// Parses a positive integer torrent id. Returns false (and leaves
// outId untouched) if `s` isn't a valid positive integer.
bool parseId(const std::string& s, int& outId) {
    if (s.empty()) return false;
    char* end = nullptr;
    long v = std::strtol(s.c_str(), &end, 10);
    if (end != s.c_str() + s.size() || v <= 0) return false;
    outId = static_cast<int>(v);
    return true;
}

void printTorrentTable(const std::vector<Torrent>& torrents) {
    if (torrents.empty()) {
        std::printf("%s\n", tr(Str::CliListEmpty));
        return;
    }
    std::printf("%-6s %-40s %6s %10s %10s %10s  %-16s %s\n",
        tr(Str::HeaderId), tr(Str::HeaderName), tr(Str::HeaderDone),
        tr(Str::HeaderSize), tr(Str::HeaderDownload), tr(Str::HeaderUpload),
        tr(Str::HeaderAdded), tr(Str::HeaderStatus));
    for (const auto& t : torrents) {
        std::string name = padOrTruncateUtf8(t.name, 40);
        // formatSize() uses an adaptive unit (KB/MB/GB/TB/...) instead of
        // always MB, so the number stays short regardless of how large
        // the torrent is — see TextUtil.h for why a fixed unit breaks
        // down (this is the same bug the TUI's list view had).
        std::string size = formatSize(t.sizeBytes);
        std::string added = formatUnixTimestamp(t.addedDate);
        std::printf("%-6d %s %5.1f%% %10s %7.0fKB/s %7.0fKB/s  %-16s %s\n",
            t.id, name.c_str(), t.percentDone * 100.0, size.c_str(),
            t.rateDownload / 1024.0, t.rateUpload / 1024.0,
            added.c_str(), trTorrentStatus(t.status));
    }
}

} // namespace

int runCli(int argc, char** argv, AppSettings settings) {
    std::vector<std::string> positional;
    bool deleteData = false;

    // Global options can appear anywhere on the command line, mixed in
    // with the subcommand and its own argument.
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--host" && i + 1 < argc) settings.host = argv[++i];
        else if (a == "--port" && i + 1 < argc) settings.port = std::atoi(argv[++i]);
        else if (a == "--user" && i + 1 < argc) settings.user = argv[++i];
        else if (a == "--password" && i + 1 < argc) settings.password = argv[++i];
        else if (a == "--delete-data") deleteData = true;
        else if (a == "--help" || a == "-h") { printUsage(); return 0; }
        else positional.push_back(a);
    }

    if (positional.empty()) {
        printUsage();
        return 1;
    }

    const std::string& cmd = positional[0];
    if (cmd == "help") { printUsage(); return 0; }

    // Built from `settings` (loaded from disk by main(), then possibly
    // overridden above): no need to type --host/--user/--password again
    // if they're already saved via the TUI's Settings window.
    TransmissionClient client(settings.host, settings.port, settings.user, settings.password);

    if (cmd == "list") {
        auto torrents = client.listTorrents();
        // listTorrents() returns an empty vector both when the daemon
        // has genuinely no torrents AND when the request failed (wrong
        // host/port, daemon down, auth failure, ...) — lastError() is
        // the only way to tell those two apart, otherwise every failure
        // would silently print "No torrents." with exit code 0.
        if (torrents.empty() && !client.lastError().empty()) {
            std::fprintf(stderr, "%s\n", client.lastError().c_str());
            return 1;
        }
        printTorrentTable(torrents);
        return 0;
    }

    if (cmd == "add") {
        if (positional.size() < 2) {
            std::fprintf(stderr, tr(Str::CliErrorMissingArgument), "add <magnet|url|path>");
            std::fprintf(stderr, "\n");
            return 1;
        }
        using AddResult = TransmissionClient::AddTorrentResult;
        AddResult addResult = client.addTorrent(positional[1]);
        if (addResult == AddResult::Added) {
            std::printf("%s\n", tr(Str::CliAddSuccess));
            return 0;
        }
        if (addResult == AddResult::Duplicate) {
            // Not treated as a script-facing failure: Transmission's own
            // RPC returns "result":"success" for this too (see
            // TransmissionClient::AddTorrentResult's comment) — it's
            // informational, not an error.
            std::printf("%s\n", tr(Str::CliAddDuplicate));
            return 0;
        }
        std::printf("%s\n", tr(Str::CliAddFailure));
        return 1;
    }

    if (cmd == "start" || cmd == "stop" || cmd == "remove") {
        if (positional.size() < 2) {
            std::string argHint = cmd + " <id>";
            std::fprintf(stderr, tr(Str::CliErrorMissingArgument), argHint.c_str());
            std::fprintf(stderr, "\n");
            return 1;
        }
        int id = 0;
        if (!parseId(positional[1], id)) {
            std::fprintf(stderr, tr(Str::CliErrorInvalidId), positional[1].c_str());
            std::fprintf(stderr, "\n");
            return 1;
        }

        bool ok = false;
        if (cmd == "start") {
            ok = client.startTorrent(id);
            std::printf("%s\n", ok ? tr(Str::CliStartSuccess) : tr(Str::CliStartFailure));
        } else if (cmd == "stop") {
            ok = client.stopTorrent(id);
            std::printf("%s\n", ok ? tr(Str::CliStopSuccess) : tr(Str::CliStopFailure));
        } else {
            ok = client.removeTorrent(id, deleteData);
            std::printf("%s\n", ok ? tr(Str::CliRemoveSuccess) : tr(Str::CliRemoveFailure));
        }
        return ok ? 0 : 1;
    }

    std::fprintf(stderr, tr(Str::CliErrorUnknownCommand), cmd.c_str());
    std::fprintf(stderr, "\n");
    printUsage();
    return 1;
}
