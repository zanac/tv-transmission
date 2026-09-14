#pragma once
#include <string>

// Pads or truncates a UTF-8 string to exactly `width` terminal COLUMNS,
// counting codepoints rather than bytes.
//
// This matters because a name with accented letters (e, a, o with
// diacritics) or other multi-byte UTF-8 characters takes more bytes than
// displayed characters. A byte-based pad/truncate (e.g. printf's
// "%-30.30s") can consume a different number of bytes for two names of
// the same visual length, shifting every column that follows by a
// variable amount depending on the name. Counting codepoints instead
// keeps every row's trailing columns aligned regardless of content.
std::string padOrTruncateUtf8(const std::string& s, size_t width);

// Formats a byte count using an adaptive unit (B/KB/MB/GB/TB/PB) so the
// numeric part stays short (one decimal, generally 1-4 digits) no
// matter how large the value is.
//
// This exists because a fixed unit with an assumed-bounded digit count
// (e.g. always MB with an 8-character field) breaks down for a large
// enough value: a ~7 TB torrent shown in MB needs 9 digits, one more
// than an 8-character field allows, which silently shifts every column
// that follows by the overflow amount. Adaptive units keep the number
// itself short across realistic sizes; right-pad/truncate the result
// with padOrTruncateUtf8() for column alignment same as any other field
// (the residual risk of a still-too-long string, e.g. multiple exabytes
// in a single torrent, is deliberately left unhandled — see the
// comment where callers use this).
std::string formatSize(int64_t bytes);

// Formats a Unix timestamp (seconds since epoch) as "YYYY-MM-DD HH:MM"
// in local time. Returns an empty string for 0 (Transmission uses 0 to
// mean "unknown"/"not set").
std::string formatUnixTimestamp(int64_t unixSeconds);

// Truncates a UTF-8 string to at most `maxWidth` terminal columns
// (counting codepoints, same reasoning as padOrTruncateUtf8() above),
// appending "..." when it actually gets cut short. Unlike
// padOrTruncateUtf8(), never pads a shorter string — meant for titles
// and labels, where trailing spaces would be wrong, not for fixed-width
// table columns.
std::string truncateUtf8(const std::string& s, size_t maxWidth);

// Formats a duration in seconds as "3h 23m", "5m 10s", or "0s" —
// whichever units are non-zero from the largest down, dropping smaller
// ones once a larger one is present (matching how most torrent clients
// show elapsed time). 0 or negative returns "0s".
std::string formatDuration(int64_t totalSeconds);
