#pragma once
#include <cstdint>
#include <string>

// Holds the torrent fields we care about from the RPC (torrent-get).
// Extendable later with more fields (peers, ratio, etc.)
struct Torrent {
    int id = 0;
    std::string name;
    int64_t sizeBytes = 0;
    double percentDone = 0.0;   // 0.0 - 1.0
    double rateDownload = 0.0;  // bytes/s
    double rateUpload = 0.0;    // bytes/s
    int status = 0;             // see the RPC's tr_torrent_activity enum
    std::string errorString;
    int64_t addedDate = 0;      // unix timestamp (seconds), RPC field "addedDate"

    // Per-torrent speed limit override. When *Limited is false, the
    // torrent has no speed limit of its own for that direction; *Limit
    // is in KB/s and is only meaningful while the matching *Limited flag
    // is true.
    //
    // honorsSessionLimits is a SEPARATE flag from Transmission's own RPC,
    // controlled independently in the UI: it decides whether the torrent
    // follows the session's global limit (see
    // TransmissionClient::getSessionLimits()) at all. A torrent can have
    // no override (*Limited=false) and still ignore the global limit if
    // honorsSessionLimits is false — these two things are not the same
    // choice, which is why the details window exposes them as separate
    // checkboxes rather than trying to infer one from the other.
    bool downloadLimited = false;
    int downloadLimit = 0;      // KB/s
    bool uploadLimited = false;
    int uploadLimit = 0;        // KB/s
    bool honorsSessionLimits = true;

    // Extended details (torrent details window)
    std::string downloadDir;     // RPC "downloadDir"
    bool isPrivate = false;      // RPC "isPrivate"
    std::string magnetLink;      // RPC "magnetLink"
    int64_t pieceCount = 0;      // RPC "pieceCount"
    int64_t pieceSize = 0;       // RPC "pieceSize", bytes
    int64_t downloadedEver = 0;  // RPC "downloadedEver", bytes, all-time total
    int64_t uploadedEver = 0;    // RPC "uploadedEver", bytes, all-time total
    double uploadRatio = 0.0;    // RPC "uploadRatio"
    int64_t activityDate = 0;    // RPC "activityDate", unix timestamp
    int64_t secondsDownloading = 0; // RPC "secondsDownloading"
    int64_t secondsSeeding = 0;     // RPC "secondsSeeding"

    // Used only to compute "availability %" (see
    // TorrentDetailsWindow.cpp) — not displayed directly. Same formula
    // used by Transmission's own official GTK/Qt clients: bytes we
    // already have (valid or not-yet-hash-checked) plus bytes we still
    // need that are available right now from connected peers, as a
    // fraction of the size we're actually trying to complete (which can
    // be less than the torrent's full size if some files are
    // deselected).
    int64_t haveValid = 0;         // RPC "haveValid"
    int64_t haveUnchecked = 0;     // RPC "haveUnchecked"
    int64_t desiredAvailable = 0;  // RPC "desiredAvailable"
    int64_t sizeWhenDone = 0;      // RPC "sizeWhenDone"

    // Now also requested by listTorrents() itself (not just
    // getTorrentDetails()) for the optional, hidden-by-default torrent
    // list columns — see TorrentListWindow::setupColumns().
    int64_t eta = -1;              // RPC "eta", seconds; Transmission's own
                                    // sentinels: -1 = not available, -2 = unknown
    int peersConnected = 0;        // RPC "peersConnected"
    int queuePosition = 0;         // RPC "queuePosition", 0-based
    int bandwidthPriority = 0;     // RPC "bandwidthPriority": -1 low, 0 normal, 1 high
    int64_t doneDate = 0;          // RPC "doneDate", unix timestamp; 0 = not completed yet
};

// One file within a torrent — from a combination of the "files" and
// "fileStats" RPC fields (see TransmissionClient::getTorrentFiles()),
// which Transmission returns as two parallel arrays rather than one
// merged one.
struct TorrentFile {
    std::string name;             // RPC "files[].name" — path relative to the torrent's own root
    int64_t length = 0;           // RPC "files[].length"
    int64_t bytesCompleted = 0;   // RPC "files[].bytesCompleted" (same as "fileStats[].bytesCompleted")
    bool wanted = true;           // RPC "fileStats[].wanted"
    int priority = 0;             // RPC "fileStats[].priority": -1 low, 0 normal, 1 high — same
                                   // convention as Torrent::bandwidthPriority above
};
