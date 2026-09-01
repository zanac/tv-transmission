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
    // torrent uses the session's default/global limit (see
    // TransmissionClient::getSessionLimits()); *Limit is in KB/s and is
    // only meaningful while the matching *Limited flag is true.
    bool downloadLimited = false;
    int downloadLimit = 0;      // KB/s
    bool uploadLimited = false;
    int uploadLimit = 0;        // KB/s
};
