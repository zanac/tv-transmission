#pragma once

#define Uses_TComboBox
#include <tvision/tv.h>
#include "../AppSettings.h"

// A combo box for picking one of the 5 supported languages, using the
// real TComboBox now available via the zanac/tvision fork this project
// points at (see "Building" in README.md) — tvision itself has no
// built-in combo/dropdown widget (see upstream issue #173), which is
// exactly what that fork adds, pending a PR. This used to be a
// hand-built TView opening a TMenuPopup instead; now it's a thin
// subclass that just supplies the 5-item TComboItem chain and a typed
// language() accessor, since TComboBox already does the rest (drawing,
// opening its own popup, keyboard/mouse handling).
class LanguageComboBox : public TComboBox {
public:
    LanguageComboBox(const TRect& bounds, Language initial);

    // TComboBox::value is the ulong the matching TComboItem was built
    // with, which the constructor below sets to each Language's own
    // numeric value — so this cast is exact, not just "hopefully in
    // range": value can only ever be one a TComboItem here was given.
    Language language() const { return static_cast<Language>(value); }
};

