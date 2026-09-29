#define Uses_TStatusLine
#define Uses_TStatusItem
#define Uses_TStatusDef
#include <tvision/tv.h>
#include "BandwidthStatusLine.h"

BandwidthStatusLine::BandwidthStatusLine(const TRect& bounds, TStatusDef& aDefs)
    : TStatusLine(bounds, aDefs) {}

void BandwidthStatusLine::setItemText(ushort command, const std::string& text) {
    // defs/items are protected in TStatusLine: only accessible from here,
    // being a subclass. TStatusDef::items and TStatusItem::next/text are
    // public though.
    bool changed = false;
    for (TStatusDef* d = defs; d; d = d->next) {
        for (TStatusItem* it = d->items; it; it = it->next) {
            if (it->command == command) {
                delete[] it->text;
                it->text = newStr(text.c_str());
                changed = true;
            }
        }
    }
    if (changed) drawView();
}
