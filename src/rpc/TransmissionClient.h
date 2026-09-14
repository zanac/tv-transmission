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
    // If a refresh is still in flight when this runs (a server removed
    // — see App::showConnectionDialog()'s own onServerRemoved — while
    // its own periodic refresh hadn't completed yet), the easy handle
    // gets detached from whichever CURLM it was added to BEFORE being
    // cleaned up — curl_multi_remove_handle() first, matching
    // startRefresh()'s own comment on why curl_ is safe to keep reusing
    // afterward in general; here specifically, it also means the multi
    // handle's own curl_multi_info_read() (see App::idle()) can never
    // report a since-destroyed client's request as "done", so nothing
    // downstream from that loop ever has to check whether the window
    // it identifies still exists — it simply never gets asked about
    // one that doesn't.
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

    // Detaches this client's own easy handle from `multi` if a refresh
    // is currently in flight, without waiting for either the request to
    // actually finish or this object's own destructor to eventually do
    // it. Used by App's own destructor to guarantee every client has
    // detached from the shared multi handle before IT is cleaned up
    // (curl's own multi-handle docs require every easy handle removed
    // first), explicitly and up front — rather than depending on
    // exactly when C++'s own implicit destruction order gets around to
    // each client's own destructor relative to the multi handle's own
    // cleanup.
    void cancelRefresh(CURLM* multi);

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
