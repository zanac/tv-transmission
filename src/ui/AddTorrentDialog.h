#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#include <tvision/tv.h>
#include <string>

// Creates the "Add torrent" dialog: an input field for a magnet link /
// .torrent URL / local path, plus a "Browse..." button that opens
// tvision's own file dialog (filtered to *.torrent) and fills the field
// in with whatever gets picked — the field itself still accepts
// anything typed directly (magnet link, URL, or a path Browse never
// touched), Browse is just a shortcut for the "local .torrent file"
// case. Returns the field's pointer in `urlField` (same reason as
// SettingsDialogFields: no reconstructing field order from the
// TGroup).
TDialog* createAddTorrentDialog(TInputLine*& urlField);

// Call after execView() == cmOK, BEFORE destroy(dialog).
std::string addTorrentDialogResult(TInputLine* urlField);
