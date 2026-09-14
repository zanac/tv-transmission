#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#define Uses_TCheckBoxes
#include <tvision/tv.h>
#include "../AppSettings.h"

// Direct pointers to the Filters dialog's fields — same reasoning as
// SettingsDialogFields (see SettingsDialog.h): read them back after
// execView() == cmOK, rather than re-deriving them from the TGroup.
struct FilterDialogFields {
    TInputLine* nameContains = nullptr;

    // One TCheckBoxes cluster, 7 items in tr_torrent_activity order
    // (0=stopped .. 6=seeding — same enumeration TorrentListWindow.cpp's
    // isStopped()/isQueued() use), matching TorrentFilter's show* fields
    // in that same order.
    TCheckBoxes* statusCheckboxes = nullptr;
};

// A "Reset" button clears the name field and re-checks every status —
// back to TorrentFilter's default (see TorrentFilter::isDefault() in
// AppSettings.h), i.e. "show everything" — without closing the dialog,
// so the effect is visible before deciding whether to confirm it.
TDialog* createFilterDialog(const TorrentFilter& current, FilterDialogFields& fields);

// Call after execView() == cmOK, BEFORE destroy(dialog).
TorrentFilter filterDialogResult(const FilterDialogFields& fields);
