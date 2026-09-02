#pragma once
#include <string>
#include <vector>
#include "Torrent.h"
#include "Tracker.h"

// Global (session-wide) speed limit state, as reported/set by
// session-get / session-set. When *Limited is false, that direction is
// unlimited (or governed only by per-torrent overrides, see
// Torrent::downloadLimited/uploadLimited); *Limit is in KB/s.
struct SessionLimits {
    bool downloadLimited = false;
    int downloadLimit = 0;  // KB/s
    bool uploadLimited = false;
    int uploadLimit = 0;    // KB/s
};

// Minimal client for Transmission's JSON RPC (transmission-daemon).
// Handles the session handshake (X-Transmission-Session-Id header) and
// the base methods: torrent-get, torrent-add, torrent-start, torrent-stop.
//
// Protocol reference:
// https://github.com/transmission/transmission/blob/main/docs/rpc-spec.md
class TransmissionClient {
public:
    // torrent-add's response distinguishes a genuinely new torrent from
    // one that was already present — the RPC puts the torrent's info
    // under a "torrent-added" key for the former, "torrent-duplicate"
    // for the latter, with "result":"success" either way (Transmission
    // doesn't treat re-adding an existing torrent as an error). Without
    // checking which key showed up, a caller has no way to tell the two
    // apart from a plain success/failure result.
    enum class AddTorrentResult { Added, Duplicate, Failed };

    TransmissionClient(std::string host, int port,
                        std::string user = "", std::string password = "");

    // Lists all torrents with their basic fields
    std::vector<Torrent> listTorrents();

    // Adds a torrent from a URL (magnet or .torrent link) or local path.
    // See AddTorrentResult above for what the result distinguishes.
    AddTorrentResult addTorrent(const std::string& urlOrPath);

    bool startTorrent(int id);
    bool stopTorrent(int id);
    bool removeTorrent(int id, bool deleteLocalData);

    // Bypasses the queue and starts immediately, even if the download
    // queue is full (torrent-start-now).
    bool startTorrentNow(int id);

    // Re-checks the torrent's local data against the piece hashes
    // (torrent-verify). Safe to call regardless of the torrent's current
    // state; Transmission queues the check itself.
    bool verifyTorrent(int id);

    // Asks the torrent's trackers for more peers right away
    // (torrent-reannounce), instead of waiting for the next scheduled
    // announce.
    bool reannounceTorrent(int id);

    // Fetches per-tracker stats for a single torrent (torrent-get with
    // the "trackerStats" field). Fetched on demand, not part of the
    // regular list refresh — this data isn't needed until the user
    // actually opens the tracker details window.
    std::vector<TrackerStat> getTrackerStats(int torrentId);

    // Fetches the extended fields shown in the torrent details window
    // (location, privacy, magnet link, piece info, all-time transfer
    // totals, ratio, activity/elapsed-time fields) for a single torrent.
    // Also on demand, for the same reason as getTrackerStats(): these
    // aren't part of listTorrents()'s lightweight fields, so opening
    // details always does one extra request rather than the periodic
    // refresh always carrying fields it rarely needs.
    Torrent getTorrentDetails(int torrentId);

    // Sets (or clears) a per-torrent speed limit override, and whether
    // the torrent honors the session's global limit at all.
    // downloadLimited=false/uploadLimited=false means "no limit of its
    // own for that direction" — whether it then follows the global
    // limit or runs unrestricted depends on honorsSessionLimits, a
    // separate Transmission flag (see the comment on Torrent::
    // honorsSessionLimits in Torrent.h): true follows the global limit,
    // false ignores it regardless of downloadLimited/uploadLimited.
    bool setTorrentSpeedLimits(int id, bool downloadLimited, int downloadLimitKBs,
                                bool uploadLimited, int uploadLimitKBs,
                                bool honorsSessionLimits);

    // Reads the session's global speed limits (session-get).
    // Fetches global (session-wide) speed limits. `ok`, if given, is set
    // to whether the fetch actually succeeded — callers that intend to
    // write these values back afterwards (see setSessionLimits()) need
    // this: on failure this still returns a default-constructed
    // SessionLimits (all disabled/zero), which is indistinguishable from
    // "the server genuinely has no limits set" unless the caller checks
    // `ok`. Sending that default back as if it were the real fetched
    // state would silently wipe out any real limits already configured
    // on the server.
    SessionLimits getSessionLimits(bool* ok = nullptr);

    // Sets the session's global speed limits (session-set).
    bool setSessionLimits(const SessionLimits& limits);

    // Reconfigures endpoint/credentials (e.g. from the settings window).
    // Invalidates the current session: it will be renegotiated on the
    // next call().
    void setEndpoint(std::string host, int port);
    void setCredentials(std::string user, std::string password);

    // Last human-readable error (network, auth, RPC)
    const std::string& lastError() const { return lastError_; }

private:
    // Performs a generic RPC request; returns the JSON response body as
    // a string, handling session-id renewal (409) internally.
    std::string call(const std::string& method, const std::string& argumentsJson);

    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string sessionId_;
    std::string lastError_;
};
