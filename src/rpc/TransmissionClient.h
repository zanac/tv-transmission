#pragma once
#include <string>
#include <vector>
#include "Torrent.h"
#include "Tracker.h"
#include "Peer.h"

// Opaque handle types from <curl/curl.h> — forward-declared here
// exactly as curl.h itself defines them ("typedef void CURL;"/"typedef
// void CURLM;") so this header doesn't need to include curl.h just to
// name pointers to them; only TransmissionClient.cpp ever actually
// dereferences one. CURLM is the "multi" handle used for the
// non-blocking refresh path (see startRefresh() below) — one shared by
// every client, owned by App itself (see its own comment on why).
typedef void CURL;
typedef void CURLM;
struct curl_slist; // forward-declared the same way, for the same reason — only ever used as a pointer here

// Global (session-wide) speed limit state, as reported/set by
// session-get / session-set. When *Limited is false, that direction is
// unlimited (or governed only by per-torrent overrides, see
// Torrent::downloadLimited/uploadLimited); *Limit is in KB/s.
struct SessionLimits {
    bool downloadLimited = false;
    int downloadLimit = 0;  // KB/s
    bool uploadLimited = false;
    int uploadLimit = 0;    // KB/s

    // The "alt speed" (a.k.a. turtle-mode) limits — a second, usually
    // lower, pair of limits Transmission switches to as a whole (via
    // altSpeedEnabled below) when e.g. you want bandwidth back during
    // the day without having to remember the normal limits to restore
    // them later. The limit values themselves are always present
    // regardless of whether the mode is currently active, same as the
    // official Transmission clients show them.
    bool altSpeedEnabled = false;
    int altSpeedDown = 0;    // KB/s
    int altSpeedUp = 0;      // KB/s
};

// From session-stats' own "current-stats"/"cumulative-stats" objects —
// "current" resets to zero each time the daemon (re)starts, "cumulative"
// never resets and is what a person usually means by "all-time" totals.
// Transmission's own daemon.stats file is what actually persists the
// cumulative side across restarts — this app has no local counterpart
// of its own to keep in sync, it only ever reads what the daemon
// already tracks.
struct SessionStats {
    int64_t currentUploadedBytes = 0;
    int64_t currentDownloadedBytes = 0;
    int64_t currentSecondsActive = 0;
    int64_t cumulativeUploadedBytes = 0;
    int64_t cumulativeDownloadedBytes = 0;
    int64_t cumulativeSecondsActive = 0;
    // How many times the daemon has been started, ever — Transmission's
    // own "sessionCount" field name for this, kept as-is rather than
    // renamed, despite how easy it'd be to misread as some OTHER count
    // (torrents, transfers) at a glance.
    int cumulativeSessionCount = 0;
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
                        std::string user = "", std::string password = "",
                        std::string rpcPath = "transmission/rpc");
    // If a refresh is still in flight when this runs (a server removed
    // — see App::showConnectionDialog()'s own onServerRemoved — while
    // its own periodic refresh hadn't completed yet), refreshCurl_ gets
    // detached from whichever CURLM it was added to BEFORE being
    // cleaned up — curl_multi_remove_handle() first, matching
    // startRefresh()'s own comment on why refreshCurl_ is safe to keep
    // reusing afterward in general; here specifically, it also means
    // the multi handle's own curl_multi_info_read() (see App::idle())
    // can never report a since-destroyed client's request as "done",
    // so nothing downstream from that loop ever has to check whether
    // the window it identifies still exists — it simply never gets
    // asked about one that doesn't.
    ~TransmissionClient();

    // Never copied or moved anywhere in this codebase (always used by
    // reference, or held in a std::unique_ptr — see App.cpp's own
    // clients_ map) — and, now that this class owns a raw CURL* handle
    // (see curl_ below), copying it would mean two objects both trying
    // to clean up the same one. Deleted rather than left implicit, so a
    // future accidental copy is a compile error instead of a
    // double-free at runtime.
    TransmissionClient(const TransmissionClient&) = delete;
    TransmissionClient& operator=(const TransmissionClient&) = delete;
    TransmissionClient(TransmissionClient&&) = delete;
    TransmissionClient& operator=(TransmissionClient&&) = delete;

    // Lists all torrents with their basic fields
    std::vector<Torrent> listTorrents();

    // Non-blocking equivalent of listTorrents(), for the periodic
    // refresh loop specifically (see App::idle()) — the one call site
    // where blocking is most disruptive, since it runs on a timer
    // across every open window rather than in direct response to
    // something the user just clicked, and an unreachable server would
    // otherwise freeze the whole app, not just its own window (see
    // "Fixed bugs" for why this exists at all). Other actions (start,
    // stop, remove, ...) stay ordinary blocking calls — each one is
    // short, a direct response to something just clicked, and now
    // bounded by call()'s own timeout either way.
    //
    // Starts the request on `multi` (a CURLM* the caller owns — see
    // App's own comment on why there's exactly one, shared) using this
    // client's own persistent easy handle; `privateData` is returned
    // unchanged by finishRefresh() below, letting the caller identify
    // which window a completed request belongs to without maintaining
    // a separate lookup of its own (see CURLOPT_PRIVATE in curl's own
    // docs). A second call while one's already in flight
    // (isRefreshInFlight() true) is a no-op, not a second concurrent
    // request on the same handle.
    void startRefresh(CURLM* multi, void* privateData);
    bool isRefreshInFlight() const { return refreshInFlight_; }

    // Call once the caller's own curl_multi_info_read() loop (see
    // App::idle()) reports THIS client's own easy handle as finished —
    // never before that, and never more than once per startRefresh().
    // Removes the easy handle from `multi` again (it stays alive and
    // reusable — see curl_'s own comment — just detached from this one
    // multi transfer), parses whatever was received, and returns the
    // same shape listTorrents() itself would have. `ok`, if given,
    // reports whether the request actually succeeded — a network
    // error, a session renewal that needs a retry next cycle instead
    // of this one, or a malformed response all count as failure, with
    // lastError() set to say why, the same as every synchronous call
    // in this class already does.
    std::vector<Torrent> finishRefresh(CURLM* multi, bool* ok = nullptr);

    // Adds a torrent from a URL (magnet or .torrent link) or local path.
    // `downloadDir`, if non-empty, overrides where THIS torrent is
    // saved (torrent-add's own "download-dir" argument) — a path on
    // the DAEMON's own filesystem, which may not be the same machine
    // this app itself is running on (see AddTorrentDialog's own
    // "Change..." button, whose folder browser only ever looks at the
    // LOCAL filesystem — see its own doc comment for why that's a
    // known, accepted gap rather than a bug: RPC has no "list a
    // directory on the daemon" method to browse the real one instead).
    // Left empty, the daemon's own default download directory applies,
    // same as before this parameter existed. See AddTorrentResult above
    // for what the result distinguishes.
    AddTorrentResult addTorrent(const std::string& urlOrPath, const std::string& downloadDir = "");

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

    // Queue reordering — Transmission processes queued (not-yet-active)
    // torrents in queue-position order; these change where a torrent
    // sits in that order relative to the others, rather than setting an
    // absolute position directly (there's no RPC call for "move to
    // position N" — only these four relative moves).
    bool queueMoveTop(int id);
    bool queueMoveUp(int id);
    bool queueMoveDown(int id);
    bool queueMoveBottom(int id);

    // Fetches per-tracker stats for a single torrent (torrent-get with
    // the "trackerStats" field). Fetched on demand, not part of the
    // regular list refresh — this data isn't needed until the user
    // actually opens the tracker details window.
    std::vector<TrackerStat> getTrackerStats(int torrentId);

    // Fetches the current peer list for a single torrent (torrent-get
    // with the "peers" field) — same "on demand, not part of the
    // regular list refresh" reasoning as getTrackerStats() above; this
    // is the Peers tab of the same window (see TrackerPeerWindow), not
    // requested until that tab is actually shown.
    std::vector<Peer> getPeers(int torrentId);

    // Fetches the extended fields shown in the torrent details window
    // (location, privacy, magnet link, piece info, all-time transfer
    // totals, ratio, activity/elapsed-time fields) for a single torrent.
    // Also on demand, for the same reason as getTrackerStats(): these
    // aren't part of listTorrents()'s lightweight fields, so opening
    // details always does one extra request rather than the periodic
    // refresh always carrying fields it rarely needs.
    Torrent getTorrentDetails(int torrentId);

    // Fetches every file within a single torrent (torrent-get with the
    // "files" and "fileStats" fields, merged into one TorrentFile per
    // index — Transmission returns them as two parallel arrays, not one
    // combined one). On demand, same reasoning as getTrackerStats():
    // not needed until the user actually opens the files window for one
    // specific torrent.
    std::vector<TorrentFile> getTorrentFiles(int torrentId);

    // Marks specific files (by their index within the torrent, matching
    // getTorrentFiles()'s own order) as wanted or not — torrent-set's
    // "files-wanted"/"files-unwanted". A file already fully downloaded
    // isn't deleted by being marked unwanted; Transmission just stops
    // caring about verifying/keeping it up to date.
    bool setFilesWanted(int torrentId, const std::vector<int>& fileIndices, bool wanted);

    // Sets the download priority (-1 low, 0 normal, 1 high — same
    // convention as Torrent::bandwidthPriority) for specific files —
    // torrent-set's "priority-low"/"priority-normal"/"priority-high".
    bool setFilesPriority(int torrentId, const std::vector<int>& fileIndices, int priority);

    // Sets the torrent's OWN bandwidth priority (-1 low, 0 normal, 1
    // high — same convention as Torrent::bandwidthPriority and the
    // `priority` parameter above) — torrent-set's own "bandwidthPriority"
    // field directly, a single value for the whole torrent rather than
    // the per-file "priority-low"/"priority-normal"/"priority-high"
    // index arrays setFilesPriority() above uses. Distinct from a speed
    // limit (see setTorrentSpeedLimits()): this only affects how
    // Transmission divides available bandwidth among torrents that are
    // all otherwise unrestricted, not an absolute KB/s cap of its own.
    bool setPriority(int torrentId, int priority);

    // Renames a file or folder within a torrent — a separate RPC method
    // of its own ("torrent-rename-path", not "torrent-set" despite the
    // similar name), because Transmission only ever renames the LAST
    // path component: `path` is the item's own CURRENT full path
    // relative to the torrent's root (same convention as
    // TorrentFile::name — a FileTreeRow's own `path` holds exactly
    // this, whether the row is a file or a folder), `newName` is just
    // the new leaf name, not a new full path. Only one torrent at a
    // time — unlike most torrent-set actions, this RPC method's own
    // spec doesn't support acting across several at once.
    bool renamePath(int torrentId, const std::string& path, const std::string& newName);

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
    // Sets the torrent's own seed ratio choice — mode 0 (follow the
    // global ratio limit), 1 (use `ratio` as this torrent's own limit),
    // or 2 (seed with no ratio limit at all); see Torrent::seedRatioMode
    // (Torrent.h) for why this is one three-way choice rather than a
    // flag. `ratio` is only sent meaningfully when mode == 1, but is
    // always included — Transmission itself ignores it for the other
    // two modes, so there's no need to omit it conditionally here.
    bool setTorrentSeedRatioLimit(int id, int mode, double ratio);

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

    // Asks the daemon to check whether its own configured incoming peer
    // port is reachable from outside (session-level RPC "port-test" —
    // there's no torrent or port argument to pass; it always tests
    // whatever port-get would currently report). `portOpen`, if given,
    // is set to the result; the return value is whether the check
    // itself completed at all (a network failure to the daemon means
    // `portOpen` was never actually determined, same "ok vs. genuine
    // false" distinction as getSessionLimits() above).
    // Fetches the daemon's own current-session and all-time (cumulative)
    // transfer totals, active-time, and start count (session-stats RPC
    // — see SessionStats' own comment on the current/cumulative
    // distinction). `ok`, if given, reports whether the call actually
    // succeeded, the same "empty result vs. genuine zero" distinction
    // getSessionLimits() already makes — a torrent-free daemon
    // genuinely reports all zeros here, which isn't itself a failure.
    SessionStats getSessionStats(bool* ok = nullptr);

    bool testPort(bool* portOpen = nullptr);

    // The daemon's own configured default download directory
    // (session-get's own "download-dir" field) — a separate round trip
    // from getSessionLimits() above rather than folding it into that
    // struct, since it isn't a limit and that struct's own callers
    // (Server Settings) have no use for it; this is only ever needed
    // when opening "Add torrent" (see AddTorrentDialog), where a single
    // extra call on a manually-opened dialog is no real cost.
    std::string getDefaultDownloadDir(bool* ok = nullptr);

    // Bytes free at `path`, on whatever filesystem the DAEMON sees it
    // on (RPC "free-space") — the same free space a torrent would
    // actually be competing for if saved there, not this app's own
    // local disk (which may well be a different machine entirely).
    // `ok`, if given, reports whether the call succeeded — `path` not
    // existing/being readable on the daemon's own side counts as
    // failure here too, same as a network error, since there's no
    // meaningful byte count to report either way.
    int64_t getFreeSpace(const std::string& path, bool* ok = nullptr);

    // Asks the daemon to re-download and reload its own IP blocklist
    // from whatever URL it's configured with (session-level RPC
    // "blocklist-update") — this app has no UI for setting that URL
    // itself, only for triggering the daemon's own already-configured
    // update. `ruleCount`, if given, is set to how many rules the
    // blocklist ended up with; the return value is whether the update
    // itself completed.
    bool updateBlocklist(int* ruleCount = nullptr);

    // Reconfigures endpoint/credentials (e.g. from the settings window).
    // Invalidates the current session: it will be renegotiated on the
    // next call().
    void setEndpoint(std::string host, int port);
    // The host currently configured — e.g. for deciding whether a
    // feature that only makes sense against a LOCAL daemon (browsing
    // this machine's own filesystem to choose a download folder) should
    // be offered at all; see AddTorrentDialog's own "Change..." button.
    const std::string& getHost() const { return host_; }
    void setCredentials(std::string user, std::string password);
    // Same reasoning as setEndpoint()/setCredentials() above — updates
    // an already-constructed client's own RPC path in place (e.g. after
    // editing an existing server's connection details, rather than
    // adding a brand new one), rather than requiring the whole client
    // object to be torn down and rebuilt just for this one field.
    void setRpcPath(std::string rpcPath);

    // Last human-readable error (network, auth, RPC)
    const std::string& lastError() const { return lastError_; }

private:
    // Performs a generic RPC request; returns the JSON response body as
    // a string, handling session-id renewal (409) internally.
    std::string call(const std::string& method, const std::string& argumentsJson);

    // Same shape as the plain functions call() itself uses (see
    // TransmissionClient.cpp's own anonymous namespace), but as static
    // member functions instead — curl's own C API needs a plain
    // function pointer, which a static member function still is (no
    // `this` of its own), while still being able to reach refreshBody_/
    // refreshSessionIdHeader_ directly via the `userdata` pointer it's
    // given back as a TransmissionClient*, since a private write path
    // that's only ever used by the async refresh itself (see
    // startRefresh() above) doesn't need call()'s own general-purpose
    // ResponseBuffer at all.
    static size_t refreshWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t refreshHeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);

    std::string host_;
    int port_;
    std::string rpcPath_;
    std::string user_;
    std::string password_;
    std::string sessionId_;
    std::string lastError_;
    // One handle per client, kept alive for the client's own whole
    // lifetime instead of a fresh curl_easy_init()/curl_easy_cleanup()
    // on every single call — lets curl reuse the underlying TCP
    // connection across calls to the same host instead of a fresh
    // handshake every time (see "Fixed bugs" below). curl_easy_reset()
    // at the start of every call() still clears out whatever options
    // the PREVIOUS call left set, so nothing carries over by accident
    // (a stale CURLOPT_POSTFIELDS pointing at a since-destroyed
    // std::string, for instance) — the only thing actually persisting
    // across calls on purpose is the connection itself.
    CURL* curl_ = nullptr;
    // A SEPARATE easy handle from curl_ above, used ONLY by startRefresh()/
    // finishRefresh() — never by call(). The two used to share curl_,
    // which is unsafe: curl_multi_add_handle() (in startRefresh()) adds
    // an easy handle to a multi transfer, and libcurl's own docs are
    // explicit that an easy handle attached to a multi handle must not
    // be used for a separate curl_easy_perform() (what call() does)
    // until it's been removed again. With one shared handle, opening
    // TrackerPeerWindow and switching between its Trackers/Peers tabs
    // quickly enough (each switch calling getTrackerStats()/getPeers(),
    // both synchronous call()s) could land while the SAME window's own
    // periodic async refresh had that same handle mid-transfer,
    // resetting and reusing it out from under curl's own multi-handle
    // bookkeeping — observed as the app getting stuck with an empty
    // list, not a clean crash, which is what use-after-free-adjacent
    // API misuse like this tends to look like rather than something
    // that fails loudly and immediately. Two handles means the two code
    // paths can never collide, at the cost of one call() from
    // getTrackerStats()/getPeers()/etc. not reusing the SAME underlying
    // TCP connection the periodic refresh's own handle already has open
    // to the same host — a second connection gets opened instead,
    // trivial next to correctness here.
    CURL* refreshCurl_ = nullptr;
    // Non-null exactly while a request started by startRefresh() is
    // still in flight — which CURLM it was added to (needed by both
    // finishRefresh(), to remove it again, and the destructor, to do
    // the same if it never got the chance to finish at all).
    CURLM* refreshMulti_ = nullptr;
    bool refreshInFlight_ = false;
    std::string refreshBody_;
    std::string refreshSessionIdHeader_;
    // CURLOPT_POSTFIELDS/CURLOPT_URL don't copy the string they're given
    // — curl just keeps the pointer — so both need to stay alive for as
    // long as the request itself does, which for the async path spans
    // however many idle() ticks it takes to finish, not just one
    // function call the way call()'s own locals only need to survive.
    std::string refreshPayload_;
    std::string refreshUrl_;
    struct curl_slist* refreshHeaders_ = nullptr;
};
