#include "TextUtil.h"
#include <vector>
#include <cstdio>
#include <ctime>

namespace {

// Byte offset of each codepoint in `s`. Shared by padOrTruncateUtf8()
// and truncateUtf8() so both agree on where a character boundary falls.
std::vector<size_t> codepointStarts(const std::string& s) {
    std::vector<size_t> charStarts;
    for (size_t i = 0; i < s.size(); ) {
        charStarts.push_back(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if ((c & 0x80) == 0x00) len = 1;      // ASCII
        else if ((c & 0xE0) == 0xC0) len = 2; // 2-byte sequence
        else if ((c & 0xF0) == 0xE0) len = 3; // 3-byte sequence
        else if ((c & 0xF8) == 0xF0) len = 4; // 4-byte sequence
        if (i + len > s.size()) len = s.size() - i; // truncated sequence: don't overrun
        i += len;
    }
    return charStarts;
}

} // namespace

std::string padOrTruncateUtf8(const std::string& s, size_t width) {
    std::vector<size_t> charStarts = codepointStarts(s);
    if (charStarts.size() <= width) {
        std::string result = s;
        result.append(width - charStarts.size(), ' ');
        return result;
    }
    return s.substr(0, charStarts[width]);
}

std::string truncateUtf8(const std::string& s, size_t maxWidth) {
    std::vector<size_t> charStarts = codepointStarts(s);
    if (charStarts.size() <= maxWidth) return s;
    if (maxWidth <= 3) return s.substr(0, charStarts[maxWidth]); // too small for "..."
    return s.substr(0, charStarts[maxWidth - 3]) + "...";
}

std::string formatSize(int64_t bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    constexpr int unitCount = 6;
    double value = static_cast<double>(bytes);
    int unitIndex = 0;
    while (value >= 1024.0 && unitIndex < unitCount - 1) {
        value /= 1024.0;
        unitIndex++;
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.1f%s", value, units[unitIndex]);
    return buf;
}

std::string formatUnixTimestamp(int64_t unixSeconds) {
    if (unixSeconds <= 0) return "";
    std::time_t t = static_cast<std::time_t>(unixSeconds);
    std::tm tmValue{};
    localtime_r(&t, &tmValue);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmValue);
    return buf;
}
