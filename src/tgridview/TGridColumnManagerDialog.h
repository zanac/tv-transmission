#pragma once

#define Uses_TDialog
#include <tvision/tv.h>
#include <string>

#include "TGridView.h"

// Every piece of text this dialog shows, with plain-English defaults —
// this module has no dependency on any particular app's translation
// system (see the top-level README.md), so it can't call into one on
// its own. An app that needs this translated passes its own strings in
// here instead; see TV Transmission's own ui/ layer for an example of
// building one of these from a `tr()`-based Str enum right before
// calling createColumnManagerDialog().
struct TGridColumnManagerLabels {
    std::string title = "Manage columns";
    std::string columnHeader = "Column";
    std::string widthHeader = "Width";
    std::string visibleHeader = "Visible";
    std::string yes = "Yes";
    std::string no = "No";
    std::string resizeButton = "Resize";
    std::string moveButton = "Move";
    std::string toggleVisibleButton = "Toggle visible";
    std::string resetButton = "Reset";
    std::string closeButton = "Close";
};

// A single consolidated window to resize, reorder, and show/hide a
// TGridView's columns — everything three separate menu entries used to
// take in TV Transmission before being folded into one (see that
// project's own README.md for the story). Built as a small "meta" grid
// on the target grid itself: one row per real column, showing its
// label/current width/visibility, using TGridView to display that — so
// building this out of the widget it manages is itself a working
// example of using TGridView for something other than a torrent list.
//
// Selecting a row and pressing "Resize" or "Move" calls straight into
// `target`'s own startKeyboardResize()/startKeyboardReorder() — the
// same interactive Left/Right/Enter/Esc loops a column header's own
// mouse/keyboard entry points already use — rather than reimplementing
// that logic here, so this dialog is really just a different way to
// pick which column those already-built interactions apply to, plus a
// toggle for visibility and a call to resetColumns() for "start over".
//
// Changes apply directly to `target` as they're made (there's no
// separate "confirm" step — each individual resize/reorder already has
// its own Enter/Esc, and toggling visibility is immediate), so the only
// button here besides "Reset" and "Close" is what's needed to pick a
// row and act on it.
TDialog* createColumnManagerDialog(TGridView* target,
                                    const TGridColumnManagerLabels& labels = {});
