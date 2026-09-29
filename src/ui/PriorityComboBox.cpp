#include "PriorityComboBox.h"
#include "Strings.h"

namespace {

// Order matches how the priority column itself sorts/displays low to
// high (see TorrentListWindow::formatPriority) — not that order matters
// for a combo box's own behavior, just for it to look consistent with
// the rest of the UI.
TComboItem* buildPriorityItems() {
    return
        new TComboItem(tr(Str::PriorityLow), (ulong)(long)-1,
        new TComboItem(tr(Str::PriorityNormal), (ulong)(long)0,
        new TComboItem(tr(Str::PriorityHigh), (ulong)(long)1, nullptr)));
}

// TComboBox's constructor takes a starting FOCUS INDEX (0/1/2 into the
// item chain above), not the priority value itself — this maps
// Transmission's -1/0/1 encoding to that index.
short indexForPriority(int priority) {
    if (priority < 0) return 0; // Low
    if (priority > 0) return 2; // High
    return 1;                   // Normal
}

} // namespace

PriorityComboBox::PriorityComboBox(const TRect& bounds, int initial)
    : TComboBox(bounds, buildPriorityItems(), indexForPriority(initial)) {}
