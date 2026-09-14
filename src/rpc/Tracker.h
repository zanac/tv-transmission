#pragma once
#include <cstdint>
#include <string>

// Holds the tracker fields we care about from the RPC's "trackerStats"
// array (part of torrent-get). Counts are -1 when Transmission doesn't
// know the value (e.g. before the first successful announce) — callers
// should show "N/A" rather than a misleading 0 or -1.
struct TrackerStat {
    std::string host;    // "announce" host:port, e.g. "open.stealth.si:80"
    int tier = 0;         // 0-based in the RPC; display as tier+1 (see the
                          // screenshot this feature is modeled on, which
                          // shows "Tier 1", "Tier 2", ...)
    int seederCount = -1;
    int leecherCount = -1;
    int downloadCount = -1;
    bool lastAnnounceSucceeded = false;
    bool hasAnnounced = false;
    std::string lastAnnounceResult; // e.g. "Success", "IPv6 connection failed"
    int64_t lastAnnounceTime = 0;   // unix timestamp, 0 = never
    int64_t nextAnnounceTime = 0;   // unix timestamp, 0 = not scheduled
};
