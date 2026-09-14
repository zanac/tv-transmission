#pragma once

#define Uses_TStatusLine
#define Uses_TStatusItem
#include <tvision/tv.h>
#include <string>

// TStatusLine that exposes a method to rewrite, at runtime, the text of
// one of its TStatusItem entries (identified by its associated command).
// Used to show "D: xxx KB/s U: yyy KB/s" in the last row.
class BandwidthStatusLine : public TStatusLine {
public:
    BandwidthStatusLine(const TRect& bounds, TStatusDef& aDefs);

    void setItemText(ushort command, const std::string& text);
};
