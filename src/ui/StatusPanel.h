#pragma once

#define Uses_TView
#define Uses_TGroup
#define Uses_TInputLine
#define Uses_TCheckBoxes
#define Uses_TEvent
#include <tvision/tv.h>

#include <functional>
#include "../AppSettings.h"

// The "Status" side panel (Window -> Panels -> Status) — a live,
// always-applied replacement for the old modal Filters dialog (see
// "Fixed bugs" in README.md for that migration): the exact same name
// field + 7 status checkboxes, but every keystroke/checkbox toggle
// calls `onFilterChanged` immediately, rather than collecting values
// for an eventual OK/Cancel. There's deliberately no Reset/OK/Cancel
// here at all — a live panel has nothing to "confirm" or "cancel back
// from", unlike the modal dialog it replaces.
//
// Standard tvision DIALOG colors (gray background, blue input line,
// cyan checkbox cluster — the same look any TDialog in this app
// already has), asked for directly after an earlier attempt just let
// everything resolve through the ordinary palette chain: that gave
// this WINDOW's own blue theme instead, since TInputLine/TCheckBoxes/
// TStaticText resolve relative to whatever they're actually embedded
// in, and this panel lives inside a TWindow (TorrentListWindow), not
// a TDialog. mapColor() below hardcodes the exact bytes a TDialog's
// own default palette (cpGrayDialog, chained to cpAppColor — see
// tvision's own dialogs.h/app.h) resolves each of these widgets' own
// requests to, computed precisely rather than guessed (see the .cpp).
class StatusPanel : public TGroup {
public:
    StatusPanel(const TRect& bounds, const TorrentFilter& initial,
                std::function<void(const TorrentFilter&)> onFilterChanged);

    void handleEvent(TEvent& event) override;
    TColorAttr mapColor(uchar) override;

    // Re-applies the current TorrentFilter's own values to the fields
    // — used when a caller changes the filter some OTHER way (there
    // isn't one yet, but e.g. a future "Reset filters" menu command
    // would need this to keep the panel's own fields in sync rather
    // than showing stale values after an external change).
    void setFilter(const TorrentFilter& filter);

private:
    TorrentFilter readFilter() const;
    void notifyChanged();

    TInputLine* nameField_ = nullptr;
    TCheckBoxes* statusBoxes_ = nullptr;
    std::function<void(const TorrentFilter&)> onFilterChanged_;
};
