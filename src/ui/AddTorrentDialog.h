#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#include <tvision/tv.h>
#include <string>

// Creates the "Add torrent" dialog: a plain input field for a magnet
// link / .torrent URL / local path, typed directly. Browsing for a
// local file is a separate, top-level action (see App::
// showAddTorrentFromFileDialog() and "Add from file..." in the Torrent
// menu) rather than a button inside this dialog — nesting a second
// modal dialog on top of this one broke rendering (see the comment on
// showAddTorrentFromFileDialog() for what exactly went wrong). Returns
// the field's pointer in `urlField` (same reason as
// SettingsDialogFields: no reconstructing field order from the
// TGroup).
TDialog* createAddTorrentDialog(TInputLine*& urlField);

// Call after execView() == cmOK, BEFORE destroy(dialog).
std::string addTorrentDialogResult(TInputLine* urlField);
