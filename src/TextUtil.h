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
