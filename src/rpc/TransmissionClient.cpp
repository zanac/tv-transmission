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
      user_(std::move(user)), password_(std::move(password)) {}

std::string TransmissionClient::call(const std::string& method,
                                      const std::string& argumentsJson) {
    CURL* curl = curl_easy_init();
    if (!curl) {
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
        ResponseBuffer resp;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!sessionId_.empty()) {
            std::string h = "X-Transmission-Session-Id: " + sessionId_;
            headers = curl_slist_append(headers, h.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &resp);
        if (!user_.empty()) {
            curl_easy_setopt(curl, CURLOPT_USERNAME, user_.c_str());
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password_.c_str());
        }

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            lastError_ = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            return "";
        }

        if (httpCode == 409) {
            // Session expired/missing: store the new id and retry
            sessionId_ = resp.sessionIdHeader;
            continue;
        }

        curl_easy_cleanup(curl);
        return resp.body;
    }

    curl_easy_cleanup(curl);
    lastError_ = "unable to obtain a valid session id";
    return "";
}

std::vector<Torrent> TransmissionClient::listTorrents() {
    std::vector<Torrent> result;
    std::string args = R"({"fields":["id","name","totalSize","percentDone",
                              "rateDownload","rateUpload","status","errorString",
                              "addedDate","downloadLimited","downloadLimit",
                              "uploadLimited","uploadLimit","honorsSessionLimits"]})";
    std::string body = call("torrent-get", args);
    if (body.empty()) return result;

    try {
        json j = json::parse(body);
        for (auto& t : j["arguments"]["torrents"]) {
            Torrent tor;
            tor.id = t.value("id", 0);
            tor.name = t.value("name", "");
            tor.sizeBytes = t.value("totalSize", 0LL);
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
            result.push_back(std::move(tor));
        }
    } catch (const std::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
    }
    return result;
}

bool TransmissionClient::addTorrent(const std::string& urlOrPath) {
    json args = {{"filename", urlOrPath}};
    std::string body = call("torrent-add", args.dump());
    if (body.empty()) return false;
    try {
        json j = json::parse(body);
        return j.value("result", "") == "success";
    } catch (...) {
        return false;
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

SessionLimits TransmissionClient::getSessionLimits() {
    SessionLimits limits;
    std::string body = call("session-get", "{}");
    if (body.empty()) return limits;
    try {
        json j = json::parse(body);
        auto& a = j["arguments"];
        limits.downloadLimited = a.value("speed-limit-down-enabled", false);
        limits.downloadLimit = a.value("speed-limit-down", 0);
        limits.uploadLimited = a.value("speed-limit-up-enabled", false);
        limits.uploadLimit = a.value("speed-limit-up", 0);
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
    };
    return !call("session-set", args.dump()).empty();
}
