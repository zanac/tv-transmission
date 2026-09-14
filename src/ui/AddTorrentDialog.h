#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#include <tvision/tv.h>
#include <string>

// Creates the "Add torrent" dialog: an input field for a magnet link /
// .torrent URL / local path, plus a "Browse..." button. Confirming with
// OK returns cmOK as usual; clicking Browse instead ends the dialog
// with cmYes (repurposing tvision's built-in Yes/No command, which
// TDialog's own handleEvent already turns into endModal() while modal —
// no custom subclass needed just for this).
//
// The caller (App::showAddTorrentDialog()) is expected to destroy this
// dialog on cmYes, THEN open TFileDialog directly from the application
// (one level of nesting, not two) and, if a file gets picked, recreate
// this same dialog with `initialValue` set to that path so the user
// still sees/can edit it before confirming. Browse isn't a button that
// opens TFileDialog *from inside* this dialog — nesting TFileDialog
// inside an already-open dialog is what broke rendering in the first
// place (see the "Fixed bugs" entry on this in README.md); closing this
// one first keeps every dialog exactly one level deep, same as
// everywhere else in this app.
TDialog* createAddTorrentDialog(TInputLine*& urlField, const std::string& initialValue = "");

// Call after execView() == cmOK, BEFORE destroy(dialog).
std::string addTorrentDialogResult(TInputLine* urlField);
