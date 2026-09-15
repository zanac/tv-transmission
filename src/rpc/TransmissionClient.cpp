#include "TransmissionClient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace {

struct ResponseBuffer {
    std::string body;
    std::string sessionIdHeader;
};

size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<ResponseBuffer*>(userdata);
    buf->body.append(ptr, size * nmemb);
    return size * nmemb;
}

size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* buf = static_cast<ResponseBuffer*>(userdata);
    std::string header(buffer, size * nitems);
    const std::string key = "X-Transmission-Session-Id:";
    auto pos = header.find(key);
    if (pos != std::string::npos) {
        std::string value = header.substr(pos + key.size());
        // trim spaces/CRLF
        size_t start = value.find_first_not_of(" \t");
        size_t end = value.find_last_not_of(" \t\r\n");
        if (start != std::string::npos)
            buf->sessionIdHeader = value.substr(start, end - start + 1);
    }
    return size * nitems;
}

} // namespace

TransmissionClient::TransmissionClient(std::string host, int port,
                                        std::string user, std::string password)
    : host_(std::move(host)), port_(port),
      user_(std::move(user)), password_(std::move(password)),
      curl_(curl_easy_init()), refreshCurl_(curl_easy_init()) {}

TransmissionClient::~TransmissionClient() {
    if (curl_) curl_easy_cleanup(curl_);
    if (refreshCurl_) {
        if (refreshInFlight_ && refreshMulti_) {
            curl_multi_remove_handle(refreshMulti_, refreshCurl_);
        }
        curl_easy_cleanup(refreshCurl_);
    }
    if (refreshHeaders_) curl_slist_free_all(refreshHeaders_);
}

std::string TransmissionClient::call(const std::string& method,
                                      const std::string& argumentsJson) {
    // Cleared at the START of every attempt, not just set on failure —
    // otherwise a successful call would leave a STALE error from some
    // earlier failed one still sitting in lastError_, and callers now
    // rely on lastError().empty() to mean "the most recent attempt
    // succeeded" (see TorrentListWindow::updateTitleForConnectionState()
    // and its own callers) — that's only true if success reliably
    // clears it too, not just failure setting it.
    lastError_.clear();

    if (!curl_) {
        lastError_ = "unable to initialize libcurl";
        return "";
    }

    json payload = {
        {"method", method},
        {"arguments", json::parse(argumentsJson.empty() ? "{}" : argumentsJson)}
    };
    std::string payloadStr = payload.dump();

    std::ostringstream urlStream;
    urlStream << "http://" << host_ << ":" << port_ << "/transmission/rpc";
    std::string url = urlStream.str();

    for (int attempt = 0; attempt < 2; ++attempt) {
        // Clears out whatever options the PREVIOUS call on this same
        // handle left set (see curl_'s own comment in the header) —
        // reusing the handle is what gets the connection reuse, but
        // every option still needs setting fresh each time, the same
        // as it would on a brand new one.
        curl_easy_reset(curl_);

        ResponseBuffer resp;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!sessionId_.empty()) {
            std::string h = "X-Transmission-Session-Id: " + sessionId_;
            headers = curl_slist_append(headers, h.c_str());
        }

        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, payloadStr.c_str());
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl_, CURLOPT_HEADERDATA, &resp);
        // Bounds how long a single call can ever block for — without
        // this, an unreachable or hung server left the whole app frozen
        // for however long the OS's own TCP-level timeout happens to
        // be (often minutes), since every RPC call runs synchronously
        // on the same thread as the rest of the UI (see "Fixed bugs"
        // below). 5s to actually connect, 15s total for the whole
        // request/response — generous for a LAN daemon under normal
        // conditions, short enough that a genuinely unreachable one
        // fails predictably instead of appearing to hang.
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 15L);
        if (!user_.empty()) {
            curl_easy_setopt(curl_, CURLOPT_USERNAME, user_.c_str());
            curl_easy_setopt(curl_, CURLOPT_PASSWORD, password_.c_str());
        }

        CURLcode res = curl_easy_perform(curl_);
        long httpCode = 0;
        curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            lastError_ = curl_easy_strerror(res);
            return "";
        }

        if (httpCode == 409) {
            // Session expired/missing: store the new id and retry
            sessionId_ = resp.sessionIdHeader;
            continue;
        }

        return resp.body;
    }

    lastError_ = "unable to obtain a valid session id";
    return "";
}

namespace {

// Shared by listTorrents() (which only requests the lightweight base
// fields, so the extended ones below just fall back to their defaults)
// and getTorrentDetails() (which requests everything). Whatever wasn't
// included in the "fields" list of the request simply isn't present in
// `t`, and .value()'s fallback handles that harmlessly either way.
Torrent parseTorrent(const json& t) {
    Torrent tor;
    tor.id = t.value("id", 0);
    tor.name = t.value("name", "");
    tor.sizeBytes = t.value("totalSize", (int64_t)0);
    tor.percentDone = t.value("percentDone", 0.0);
    tor.rateDownload = t.value("rateDownload", 0.0);
    tor.rateUpload = t.value("rateUpload", 0.0);
    tor.status = t.value("status", 0);
    tor.errorString = t.value("errorString", "");
    tor.addedDate = t.value("addedDate", (int64_t)0);
    tor.downloadLimited = t.value("downloadLimited", false);
    tor.downloadLimit = t.value("downloadLimit", 0);
    tor.uploadLimited = t.value("uploadLimited", false);
    tor.uploadLimit = t.value("uploadLimit", 0);
    tor.honorsSessionLimits = t.value("honorsSessionLimits", true);
    tor.downloadDir = t.value("downloadDir", "");
    tor.isPrivate = t.value("isPrivate", false);
    tor.magnetLink = t.value("magnetLink", "");
    tor.pieceCount = t.value("pieceCount", (int64_t)0);
    tor.pieceSize = t.value("pieceSize", (int64_t)0);
    tor.downloadedEver = t.value("downloadedEver", (int64_t)0);
    tor.uploadedEver = t.value("uploadedEver", (int64_t)0);
    tor.uploadRatio = t.value("uploadRatio", 0.0);
    tor.activityDate = t.value("activityDate", (int64_t)0);
    tor.secondsDownloading = t.value("secondsDownloading", (int64_t)0);
    tor.secondsSeeding = t.value("secondsSeeding", (int64_t)0);
    tor.haveValid = t.value("haveValid", (int64_t)0);
    tor.haveUnchecked = t.value("haveUnchecked", (int64_t)0);
    tor.desiredAvailable = t.value("desiredAvailable", (int64_t)0);
    tor.sizeWhenDone = t.value("sizeWhenDone", (int64_t)0);
    tor.eta = t.value("eta", (int64_t)-1);
    tor.peersConnected = t.value("peersConnected", 0);
    tor.queuePosition = t.value("queuePosition", 0);
    tor.bandwidthPriority = t.value("bandwidthPriority", 0);
    tor.doneDate = t.value("doneDate", (int64_t)0);
    return tor;
}

} // namespace

std::vector<Torrent> TransmissionClient::listTorrents() {
    std::vector<Torrent> result;
    // The extra fields here (uploadRatio through doneDate) are for the
    // optional, hidden-by-default columns (see TorrentListWindow::
    // setupColumns()) — requested on every periodic refresh, unlike
    // getTorrentDetails()'s own fields, because a column has to be able
    // to show current data the moment it's made visible, not only after
    // the details window happens to have been opened once.
    std::string args = R"({"fields":["id","name","totalSize","percentDone",
                              "rateDownload","rateUpload","status","errorString",
                              "addedDate","downloadLimited","downloadLimit",
                              "uploadLimited","uploadLimit","honorsSessionLimits",
                              "uploadRatio","uploadedEver","downloadedEver","downloadDir",
                              "eta","peersConnected","queuePosition","bandwidthPriority",
                              "doneDate"]})";
    std::string body = call("torrent-get", args);
    if (body.empty()) return result;

    try {
        json j = json::parse(body);
        for (auto& t : j["arguments"]["torrents"])
            result.push_back(parseTorrent(t));
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return result;
}

size_t TransmissionClient::refreshWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* self = static_cast<TransmissionClient*>(userdata);
    self->refreshBody_.append(ptr, size * nmemb);
    return size * nmemb;
}

size_t TransmissionClient::refreshHeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* self = static_cast<TransmissionClient*>(userdata);
    std::string header(buffer, size * nitems);
    const std::string key = "X-Transmission-Session-Id:";
    auto pos = header.find(key);
    if (pos != std::string::npos) {
        std::string value = header.substr(pos + key.size());
        size_t start = value.find_first_not_of(" \t");
        size_t end = value.find_last_not_of(" \t\r\n");
        if (start != std::string::npos)
            self->refreshSessionIdHeader_ = value.substr(start, end - start + 1);
    }
    return size * nitems;
}

void TransmissionClient::startRefresh(CURLM* multi, void* privateData) {
    if (!refreshCurl_ || refreshInFlight_) return; // no handle to use, or one already running — see isRefreshInFlight()'s own doc comment

    curl_easy_reset(refreshCurl_); // same reasoning as call() itself — see refreshCurl_'s own comment in the header
    refreshBody_.clear();
    refreshSessionIdHeader_.clear();
    lastError_.clear(); // same reasoning as call() itself — see its own comment
    if (refreshHeaders_) { curl_slist_free_all(refreshHeaders_); refreshHeaders_ = nullptr; }

    // Same fields, same method, as the synchronous listTorrents() above
    // — this is that same request, just started without waiting for it.
    std::string args = R"({"fields":["id","name","totalSize","percentDone",
                              "rateDownload","rateUpload","status","errorString",
                              "addedDate","downloadLimited","downloadLimit",
                              "uploadLimited","uploadLimit","honorsSessionLimits",
                              "uploadRatio","uploadedEver","downloadedEver","downloadDir",
                              "eta","peersConnected","queuePosition","bandwidthPriority",
                              "doneDate"]})";
    json payload = {{"method", "torrent-get"}, {"arguments", json::parse(args)}};
    // Kept alive on this object (not a local variable) across however
    // many idle() ticks the request actually takes to finish — unlike
    // call()'s own payloadStr, which only needs to survive one
    // synchronous curl_easy_perform() on the same call stack.
    refreshPayload_ = payload.dump();

    refreshHeaders_ = curl_slist_append(refreshHeaders_, "Content-Type: application/json");
    if (!sessionId_.empty()) {
        std::string h = "X-Transmission-Session-Id: " + sessionId_;
        refreshHeaders_ = curl_slist_append(refreshHeaders_, h.c_str());
    }

    std::ostringstream urlStream;
    urlStream << "http://" << host_ << ":" << port_ << "/transmission/rpc";
    refreshUrl_ = urlStream.str();

    curl_easy_setopt(refreshCurl_, CURLOPT_URL, refreshUrl_.c_str());
    curl_easy_setopt(refreshCurl_, CURLOPT_POST, 1L);
    curl_easy_setopt(refreshCurl_, CURLOPT_POSTFIELDS, refreshPayload_.c_str());
    curl_easy_setopt(refreshCurl_, CURLOPT_HTTPHEADER, refreshHeaders_);
    curl_easy_setopt(refreshCurl_, CURLOPT_WRITEFUNCTION, refreshWriteCallback);
    curl_easy_setopt(refreshCurl_, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(refreshCurl_, CURLOPT_HEADERFUNCTION, refreshHeaderCallback);
    curl_easy_setopt(refreshCurl_, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(refreshCurl_, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(refreshCurl_, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(refreshCurl_, CURLOPT_PRIVATE, privateData);
    if (!user_.empty()) {
        curl_easy_setopt(refreshCurl_, CURLOPT_USERNAME, user_.c_str());
        curl_easy_setopt(refreshCurl_, CURLOPT_PASSWORD, password_.c_str());
    }

    curl_multi_add_handle(multi, refreshCurl_);
    refreshMulti_ = multi;
    refreshInFlight_ = true;
}

std::vector<Torrent> TransmissionClient::finishRefresh(CURLM* multi, bool* ok) {
    std::vector<Torrent> result;
    if (ok) *ok = false;
    if (!refreshInFlight_) return result; // called out of turn — see this method's own doc comment

    long httpCode = 0;
    curl_easy_getinfo(refreshCurl_, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_multi_remove_handle(multi, refreshCurl_);
    refreshMulti_ = nullptr;
    refreshInFlight_ = false;

    if (httpCode == 409) {
        // Session expired/missing: store the new id for NEXT cycle's
        // own startRefresh() to pick up — unlike call()'s own retry
        // loop, this doesn't retry within the same request: refresh
        // runs on its own short timer anyway (see App::idle()), so a
        // skipped cycle here corrects itself on the very next one
        // rather than needing its own retry machinery.
        sessionId_ = refreshSessionIdHeader_;
        lastError_ = "session renewed — retrying next refresh";
        return result;
    }

    if (refreshBody_.empty()) {
        lastError_ = "empty response";
        return result;
    }

    try {
        json j = json::parse(refreshBody_);
        for (auto& t : j["arguments"]["torrents"])
            result.push_back(parseTorrent(t));
        if (ok) *ok = true;
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return result;
}

Torrent TransmissionClient::getTorrentDetails(int torrentId) {
    // Deliberately a separate, on-demand call rather than folding these
    // fields into listTorrents()'s regular periodic refresh — same
    // reasoning as getTrackerStats(): this data isn't needed until the
    // user actually opens the details window for one specific torrent,
    // so there's no reason to fetch and parse it for every torrent on
    // every refresh tick.
    Torrent result;
    result.id = torrentId;
    json args = {
        {"ids", json::array({torrentId})},
        {"fields", json::array({
            "id", "name", "totalSize", "percentDone", "rateDownload", "rateUpload",
            "status", "errorString", "addedDate", "downloadLimited", "downloadLimit",
            "uploadLimited", "uploadLimit", "honorsSessionLimits",
            "downloadDir", "isPrivate", "magnetLink", "pieceCount", "pieceSize",
            "downloadedEver", "uploadedEver", "uploadRatio", "activityDate",
            "secondsDownloading", "secondsSeeding",
            "haveValid", "haveUnchecked", "desiredAvailable", "sizeWhenDone",
        })}
    };
    std::string body = call("torrent-get", args.dump());
    if (body.empty()) return result;

    try {
        json j = json::parse(body);
        auto& torrents = j["arguments"]["torrents"];
        if (!torrents.empty()) result = parseTorrent(torrents[0]);
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return result;
}

std::vector<TorrentFile> TransmissionClient::getTorrentFiles(int torrentId) {
    std::vector<TorrentFile> result;
    json args = {
        {"ids", json::array({torrentId})},
        {"fields", json::array({"files", "fileStats"})},
    };
    std::string body = call("torrent-get", args.dump());
    if (body.empty()) return result;

    try {
        json j = json::parse(body);
        auto& torrents = j["arguments"]["torrents"];
        if (torrents.empty()) return result;
        auto& files = torrents[0]["files"];
        auto& fileStats = torrents[0]["fileStats"];
        // "files" and "fileStats" are two parallel arrays — same length,
        // same order, one entry per file — not one combined array, so
        // they're zipped together here by index rather than each read
        // independently.
        for (size_t i = 0; i < files.size(); i++) {
            TorrentFile f;
            f.name = files[i].value("name", "");
            f.length = files[i].value("length", (int64_t)0);
            f.bytesCompleted = files[i].value("bytesCompleted", (int64_t)0);
            if (i < fileStats.size()) {
                f.wanted = fileStats[i].value("wanted", true);
                f.priority = fileStats[i].value("priority", 0);
            }
            result.push_back(std::move(f));
        }
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return result;
}

bool TransmissionClient::setFilesWanted(int torrentId, const std::vector<int>& fileIndices, bool wanted) {
    json args = {
        {"ids", json::array({torrentId})},
        {wanted ? "files-wanted" : "files-unwanted", fileIndices},
    };
    return !call("torrent-set", args.dump()).empty();
}

bool TransmissionClient::setFilesPriority(int torrentId, const std::vector<int>& fileIndices, int priority) {
    const char* field = priority < 0 ? "priority-low" : priority > 0 ? "priority-high" : "priority-normal";
    json args = {
        {"ids", json::array({torrentId})},
        {field, fileIndices},
    };
    return !call("torrent-set", args.dump()).empty();
}

bool TransmissionClient::renamePath(int torrentId, const std::string& path, const std::string& newName) {
    json args = {
        {"ids", json::array({torrentId})},
        {"path", path},
        {"name", newName},
    };
    return !call("torrent-rename-path", args.dump()).empty();
}

TransmissionClient::AddTorrentResult TransmissionClient::addTorrent(const std::string& urlOrPath) {
    json args = {{"filename", urlOrPath}};
    std::string body = call("torrent-add", args.dump());
    // If body is empty, call() has already set lastError_ (curl/network
    // failure, or a bad session handshake) — nothing more to add here.
    if (body.empty()) return AddTorrentResult::Failed;
    try {
        json j = json::parse(body);
        std::string result = j.value("result", "");
        if (result != "success") {
            // Transmission's own error text for this request — e.g.
            // "invalid or corrupt torrent file" for a bad magnet/local
            // file, or a fetch error for an unreachable http(s) URL.
            // Distinct from a network/RPC-level failure (which never
            // reaches this branch: call() already returned a non-empty
            // body precisely because the HTTP request itself succeeded).
            lastError_ = result.empty() ? "Transmission reported an error" : result;
            return AddTorrentResult::Failed;
        }
        auto& a = j["arguments"];
        if (a.contains("torrent-duplicate")) return AddTorrentResult::Duplicate;
        if (a.contains("torrent-added")) return AddTorrentResult::Added;
        lastError_ = "unexpected torrent-add response";
        return AddTorrentResult::Failed; // "success" but neither key present
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
        return AddTorrentResult::Failed;
    }
}

bool TransmissionClient::startTorrent(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("torrent-start", args.dump()).empty();
}

bool TransmissionClient::stopTorrent(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("torrent-stop", args.dump()).empty();
}

bool TransmissionClient::removeTorrent(int id, bool deleteLocalData) {
    json args = {{"ids", json::array({id})}, {"delete-local-data", deleteLocalData}};
    return !call("torrent-remove", args.dump()).empty();
}

bool TransmissionClient::startTorrentNow(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("torrent-start-now", args.dump()).empty();
}

bool TransmissionClient::verifyTorrent(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("torrent-verify", args.dump()).empty();
}

bool TransmissionClient::reannounceTorrent(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("torrent-reannounce", args.dump()).empty();
}

bool TransmissionClient::queueMoveTop(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("queue-move-top", args.dump()).empty();
}

bool TransmissionClient::queueMoveUp(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("queue-move-up", args.dump()).empty();
}

bool TransmissionClient::queueMoveDown(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("queue-move-down", args.dump()).empty();
}

bool TransmissionClient::queueMoveBottom(int id) {
    json args = {{"ids", json::array({id})}};
    return !call("queue-move-bottom", args.dump()).empty();
}

std::vector<TrackerStat> TransmissionClient::getTrackerStats(int torrentId) {
    std::vector<TrackerStat> result;
    json args = {{"ids", json::array({torrentId})}, {"fields", json::array({"trackerStats"})}};
    std::string body = call("torrent-get", args.dump());
    if (body.empty()) return result;

    try {
        json j = json::parse(body);
        auto& torrents = j["arguments"]["torrents"];
        if (torrents.empty()) return result;
        for (auto& s : torrents[0]["trackerStats"]) {
            TrackerStat t;
            t.host = s.value("host", "");
            t.tier = s.value("tier", 0);
            t.seederCount = s.value("seederCount", -1);
            t.leecherCount = s.value("leecherCount", -1);
            t.downloadCount = s.value("downloadCount", -1);
            t.lastAnnounceSucceeded = s.value("lastAnnounceSucceeded", false);
            t.hasAnnounced = s.value("hasAnnounced", false);
            t.lastAnnounceResult = s.value("lastAnnounceResult", "");
            t.lastAnnounceTime = s.value("lastAnnounceTime", (int64_t)0);
            t.nextAnnounceTime = s.value("nextAnnounceTime", (int64_t)0);
            result.push_back(std::move(t));
        }
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return result;
}

std::vector<Peer> TransmissionClient::getPeers(int torrentId) {
    std::vector<Peer> result;
    json args = {{"ids", json::array({torrentId})}, {"fields", json::array({"peers"})}};
    std::string body = call("torrent-get", args.dump());
    if (body.empty()) return result;

    try {
        json j = json::parse(body);
        auto& torrents = j["arguments"]["torrents"];
        if (torrents.empty()) return result;
        for (auto& p : torrents[0]["peers"]) {
            Peer peer;
            peer.address = p.value("address", "");
            peer.port = p.value("port", 0);
            peer.clientName = p.value("clientName", "");
            peer.progress = p.value("progress", 0.0);
            peer.rateToClient = p.value("rateToClient", (int64_t)0);
            peer.rateToPeer = p.value("rateToPeer", (int64_t)0);
            peer.flagStr = p.value("flagStr", "");
            peer.isEncrypted = p.value("isEncrypted", false);
            peer.isIncoming = p.value("isIncoming", false);
            result.push_back(std::move(peer));
        }
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return result;
}

void TransmissionClient::setEndpoint(std::string host, int port) {
    host_ = std::move(host);
    port_ = port;
    sessionId_.clear(); // the previous session is no longer valid
}

void TransmissionClient::setCredentials(std::string user, std::string password) {
    user_ = std::move(user);
    password_ = std::move(password);
    sessionId_.clear();
}

bool TransmissionClient::setTorrentSpeedLimits(int id, bool downloadLimited, int downloadLimitKBs,
                                                bool uploadLimited, int uploadLimitKBs,
                                                bool honorsSessionLimits) {
    json args = {
        {"ids", json::array({id})},
        {"downloadLimited", downloadLimited},
        {"downloadLimit", downloadLimitKBs},
        {"uploadLimited", uploadLimited},
        {"uploadLimit", uploadLimitKBs},
        {"honorsSessionLimits", honorsSessionLimits},
    };
    return !call("torrent-set", args.dump()).empty();
}

SessionLimits TransmissionClient::getSessionLimits(bool* ok) {
    SessionLimits limits;
    if (ok) *ok = false;
    std::string body = call("session-get", "{}");
    if (body.empty()) return limits;
    try {
        json j = json::parse(body);
        auto& a = j["arguments"];
        limits.downloadLimited = a.value("speed-limit-down-enabled", false);
        limits.downloadLimit = a.value("speed-limit-down", 0);
        limits.uploadLimited = a.value("speed-limit-up-enabled", false);
        limits.uploadLimit = a.value("speed-limit-up", 0);
        limits.altSpeedDown = a.value("alt-speed-down", 0);
        limits.altSpeedUp = a.value("alt-speed-up", 0);
        limits.altSpeedEnabled = a.value("alt-speed-enabled", false);
        if (ok) *ok = true;
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return limits;
}

bool TransmissionClient::setSessionLimits(const SessionLimits& limits) {
    json args = {
        {"speed-limit-down-enabled", limits.downloadLimited},
        {"speed-limit-down", limits.downloadLimit},
        {"speed-limit-up-enabled", limits.uploadLimited},
        {"speed-limit-up", limits.uploadLimit},
        {"alt-speed-down", limits.altSpeedDown},
        {"alt-speed-up", limits.altSpeedUp},
        {"alt-speed-enabled", limits.altSpeedEnabled},
    };
    return !call("session-set", args.dump()).empty();
}
