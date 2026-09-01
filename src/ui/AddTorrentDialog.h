#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#include <tvision/tv.h>
#include <string>

// Creates the "Add torrent" dialog with an input field for a magnet
// link / .torrent URL / local path, and returns its pointer in
// `urlField` (same reason as SettingsDialogFields: no reconstructing
// field order from the TGroup).
TDialog* createAddTorrentDialog(TInputLine*& urlField);

// Call after execView() == cmOK, BEFORE destroy(dialog).
std::string addTorrentDialogResult(TInputLine* urlField);
