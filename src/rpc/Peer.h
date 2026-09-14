#pragma once
#include <cstdint>
#include <string>

// Holds the peer fields we care about from the RPC's "peers" array (part
// of torrent-get, like "trackerStats" — see TrackerStat's own comment).
// Unlike trackers, Transmission doesn't report a "count unknown" sentinel
// for any of these — a peer either is or isn't in the list, with whatever
// the daemon currently knows about it.
struct Peer {
    std::string address;      // RPC "peers[].address" — bare IP, no port
    int port = 0;              // RPC "peers[].port"
    std::string clientName;    // RPC "peers[].clientName" — e.g. "qBittorrent/4.6.0"
    double progress = 0.0;     // RPC "peers[].progress" — 0.0 to 1.0
    int64_t rateToClient = 0;  // RPC "peers[].rateToClient" — bytes/sec, download FROM this peer
    int64_t rateToPeer = 0;    // RPC "peers[].rateToPeer" — bytes/sec, upload TO this peer
    // RPC "peers[].flagStr" — Transmission's own compact per-peer status
    // string (e.g. "TEHd"): shown as-is rather than decoded field-by-field,
    // the same way transmission-remote and other clients typically do —
    // see https://github.com/transmission/transmission/blob/main/docs/rpc-spec.md
    // for what each letter means.
    std::string flagStr;
    bool isEncrypted = false;  // RPC "peers[].isEncrypted"
    bool isIncoming = false;   // RPC "peers[].isIncoming"
};
