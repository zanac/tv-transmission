#include "TextUtil.h"
#include <vector>

std::string padOrTruncateUtf8(const std::string& s, size_t width) {
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
    if (charStarts.size() <= width) {
        std::string result = s;
        result.append(width - charStarts.size(), ' ');
        return result;
    }
    return s.substr(0, charStarts[width]);
}
