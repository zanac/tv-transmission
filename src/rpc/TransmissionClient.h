#pragma once
#include <string>
#include <vector>
#include "Torrent.h"

// Minimal client for Transmission's JSON RPC (transmission-daemon).
// Handles the session handshake (X-Transmission-Session-Id header) and
// the base methods: torrent-get, torrent-add, torrent-start, torrent-stop.
//
// Protocol reference:
// https://github.com/transmission/transmission/blob/main/docs/rpc-spec.md
class TransmissionClient {
public:
    TransmissionClient(std::string host, int port,
                        std::string user = "", std::string password = "");

    // Lists all torrents with their basic fields
    std::vector<Torrent> listTorrents();

    // Adds a torrent from a URL (magnet or .torrent link) or local path
    bool addTorrent(const std::string& urlOrPath);

    bool startTorrent(int id);
    bool stopTorrent(int id);
    bool removeTorrent(int id, bool deleteLocalData);

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
