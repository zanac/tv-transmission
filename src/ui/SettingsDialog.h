#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#define Uses_TRadioButtons
#include <tvision/tv.h>
#include "../AppSettings.h"

// Direct pointers to the Settings dialog's input fields.
//
// IMPORTANT: these fields used to be "found" after the dialog was
// created by scanning the TGroup's child list in the order encountered
// starting from `last`. That order does NOT match insertion order:
// TGroup::insert() inserts every new view at the head of the circular
// list, so scanning it gives the REVERSE of insertion order (verified
// empirically). With 5 fields, the result was that host/user and
// refresh/password ended up swapped with each other (the port, being
// the middle field, happened to look correct by pure coincidence). To
// eliminate this whole class of bug, the dialog returns pointers to the
// fields here at creation time, instead of having to "guess" them
// afterwards.
struct SettingsDialogFields {
    TInputLine* refreshInterval = nullptr;
    TInputLine* host = nullptr;
    TInputLine* port = nullptr;
    TInputLine* user = nullptr;
    TInputLine* password = nullptr;
    TRadioButtons* language = nullptr; // index 0 = English, 1 = Italian
};

// Creates the "Settings" dialog pre-filled with `current`'s values, and
// populates `fields` with pointers to each individual field.
TDialog* createSettingsDialog(const AppSettings& current, SettingsDialogFields& fields);

// Call after execView() == cmOK, BEFORE destroy(dialog) (otherwise the
// pointers in `fields` are no longer valid). If the refresh interval or
// port aren't numeric (or are <= 0), keeps the previous value.
AppSettings settingsDialogResult(const SettingsDialogFields& fields, const AppSettings& current);
