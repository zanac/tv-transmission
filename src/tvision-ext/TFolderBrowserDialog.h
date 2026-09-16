#pragma once

#define Uses_TDialog
#include <tvision/tv.h>
#include <string>

#include "TGridView.h"

// Every piece of text this dialog shows, with plain-English defaults —
// this module has no dependency on any particular app's translation
// system, the same reasoning as TGridColumnManagerLabels (see
// TGridColumnManagerDialog.h): an app that needs this translated
// builds one of these from its own tr()-based strings right before
// calling createFolderBrowserDialog().
struct TFolderBrowserLabels {
    std::string title = "Select Folder";
    std::string pathLabel = "Path:";
    std::string selectButton = "Select";
    std::string cancelButton = "Cancel";
    // Shown in place of the folder list when the current path can't be
    // read at all (permission denied, or it stopped existing between
    // one navigation and the next) — see TFolderBrowserDialog.cpp's own
    // comment on why this is a row of text rather than a messageBox.
    std::string unreadableDirectory = "(cannot read this directory)";
};

// A minimal, modern folder-picker — built from scratch rather than
// adapted from tvision's own TChDirDialog, which turned out to be a
// dead end for this exact purpose (see TV Transmission's own README.md
// for the full story: outdated Windows-path assumptions baked into its
// public API, a still-open memory-safety report against it, and even
// the person who tried adapting it for "choose a folder" specifically
// gave up and wrote a new one instead — the same conclusion reached
// here). Pure Unix paths throughout, std::string rather than any
// MAXPATH-limited buffer.
//
// Deliberately minimal: no tree view, no "New Folder" — just a single-
// level navigator, the same "list the current directory, double-click
// or Enter to descend, something to go back up" model tvision's own
// TFileDialog already uses for picking a FILE, applied here to picking
// a DIRECTORY instead. Built on TGridView (this project's own generic
// list widget — see TGridView.h) rather than tvision's TOutline, for
// the same reason TV Transmission's own file tree is (see its
// TorrentFilesWindow) — one consistent look across every list in an
// app that embeds this, rather than a second, differently-styled list
// widget appearing only here.
//
// `initialPath` seeds the starting directory — falls back to the
// current working directory if empty, or if it turns out not to be a
// readable directory at all. The path field at the top is directly
// editable: typing a path and pressing Enter navigates there (not the
// same as confirming the dialog — that's "Select" alone, or Enter
// while a button itself has focus), the same way a file dialog's own
// filename field lets you type ahead instead of only clicking through.
// The list itself always shows ".." (to go up a level) unless already
// at the filesystem root, followed by every readable subdirectory of
// the current path in alphabetical order — files are never listed,
// this only ever navigates directories.
TDialog* createFolderBrowserDialog(const std::string& initialPath,
                                    const TFolderBrowserLabels& labels = TFolderBrowserLabels());

// Call after execView() == cmOK, BEFORE destroy(dialog) — returns
// whatever path was showing in the path field at the moment "Select"
// confirmed it (already validated as an existing, readable directory
// by that point; see TFolderBrowserDialog.cpp's own comment on why
// confirming an unreadable/nonexistent path isn't possible in the
// first place).
std::string folderBrowserResult(TDialog* dialog);
