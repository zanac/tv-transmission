#pragma once

#define Uses_TDialog
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TStaticText
#include <tvision/tv.h>
#include <string>
#include <vector>

// A simple directory browser for picking a .torrent file, built from
// scratch instead of using tvision's own TFileDialog: TFileDialog looked
// broken when opened nested inside our own "Add torrent" dialog (a
// TDialog inside a TDialog) — its colors came out wrong (a jarring red,
// not tvision's normal palette), most likely a palette-resolution quirk
// specific to nesting dialogs two levels deep, which even tvision's own
// "tvedit" example never does (it opens TFileDialog directly from the
// application, one level deep). Sidestepping the whole class of nested-
// dialog palette issues was simpler than chasing that down. This is
// deliberately minimal: no wildcard field, no history dropdown, no info
// pane — just the current path, a list (directories first, then
// .torrent files), and Cancel. Double-clicking a directory navigates
// into it; double-clicking a file selects it and closes the dialog.
TDialog* createFileBrowserDialog(const std::string& startDir);

// Call after execView() == cmOK, BEFORE destroy(dialog). Empty if
// nothing was actually chosen (shouldn't happen if execView() returned
// cmOK, but checked defensively).
std::string fileBrowserDialogResult(TDialog* dlg);
