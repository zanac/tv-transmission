#pragma once

#define Uses_TDialog
#define Uses_TWindow
#include <tvision/tv.h>
#include "../rpc/Tracker.h"

// Small window with a tracker's full status (error message, last/next
// announce times) — the parts that don't fit in TrackerListWindow's
// table rows. A snapshot at the moment it's opened, like
// TorrentDetailsWindow's informational fields; only has a Close button,
// nothing to apply.
TWindow* createTrackerDetailWindow(const TrackerStat& t);
