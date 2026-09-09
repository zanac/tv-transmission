#pragma once

// Vendored copy of TComboBox and its three supporting classes
// (TComboItem, TComboViewer, TComboWindow) — a single-line,
// single-selection drop-down combo box that tvision itself has never
// had (see upstream issue magiblot/tvision#173). Originally added to
// this project's own tvision fork (https://github.com/zanac/tvision,
// see external/tvision in this repo) with a PR to magiblot/tvision in
// mind; copied in here instead of continuing to develop it only in the
// fork, so it can keep evolving directly as part of this project
// without every change needing to go through the fork first. The fork
// itself is unchanged and this project still depends on it for its
// other fixes (see README.md's own "Fixed bugs" section) — this is
// specifically about not being blocked on the fork for combo box work
// going forward.
//
// Ordinary #pragma once, not tvision's own Uses_X/__TComboBox
// include-guard convention: these classes are no longer declared
// anywhere in <tvision/tv.h> at all (nothing here defines
// Uses_TComboBox and friends) — LanguageComboBox.h, the only place
// that used to, now includes this header directly instead.
//
// Kept as a single header+source pair rather than split across three
// files matching the original tcombobo.cpp/tcmbovie.cpp/tcmbowin.cpp
// (mirroring how every other small standalone widget in this project —
// see PasswordInputLine.h, LanguageComboBox.h — is organized), since
// preserving upstream's own file boundaries stops mattering once the
// goal is local evolution rather than eventually diffing back against
// a PR.

#define Uses_TView
#define Uses_TWindow
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TEvent
#include <tvision/tv.h>

// A node in a singly-linked list of combo box entries. Each entry has
// a displayed `text` and an opaque `value` the caller may use to
// identify the entry (e.g. an enum or an index into another array);
// `value` may be left as 0 if unused.
//
// Ownership of the whole chain is transferred to the TComboBox it is
// passed to, which deletes it in its own destructor. A TComboWindow
// only borrows the chain for as long as its popup is on screen and
// does not delete it.
class TComboItem {
public:
    TComboItem(TStringView aText, ulong aValue, TComboItem* aNext) noexcept :
        value(aValue), next(aNext) { text = newStr(aText); }
    ~TComboItem() { delete[] (char*)text; }

    const char* text;
    ulong value;
    TComboItem* next;
};

// The scrollable list inside a TComboWindow's popup — one row per
// TComboItem. Not used standalone; TComboWindow owns one.
//
// Palette layout: 1=Active 2=Inactive 3=Focused 4=Selected 5=Divider
class TComboViewer : public TListViewer {
public:
    TComboViewer(const TRect& bounds, TScrollBar* aVScrollBar,
                 TComboItem* aItems, short aFocused) noexcept;

    TPalette& getPalette() const override;
    void getText(char* dest, short item, short maxChars) override;
    void handleEvent(TEvent& event) override;
    ulong getValue(short item) noexcept;
    int itemWidth() noexcept;

protected:
    TComboItem* items;
};

// The borderless popup list a TComboBox opens — built the same way
// THistoryWindow is for TInputLine. An entry can be chosen with the
// mouse, Enter, or double-click; Esc or a click outside the popup
// cancels it (ends modal with cmCancel).
//
// Palette layout: 1=Frame passive 2=Frame active 3=Frame icon
// 4=ScrollBar page area 5=ScrollBar controls 6=ComboViewer normal text
// 7=ComboViewer selected text
class TComboWindow : public TWindow {
public:
    TComboWindow(const TRect& bounds, TComboItem* aItems, short aFocused) noexcept;

    TPalette& getPalette() const override;
    virtual short getSelection();
    void handleEvent(TEvent& event) override;

protected:
    TComboViewer* viewer;
};

// Broadcast when the focused item actually changes as a result of the
// user picking a different one from the popup (see TComboBox::
// handleEvent()) — not fired for every open/close of the popup, only
// when the selection itself moved. cmComboBoxSelectionChanged itself
// isn't declared here: the fork this was copied from added it directly
// to views.h's own command enum (unconditionally, not gated behind any
// Uses_TComboBox guard — see cmdcodes.h/views.h), so it's already
// available from <tvision/tv.h> without needing anything specific to
// this file included first.

// A single-line, single-selection drop-down combo box. Clicking it, or
// pressing Space/Enter/Down while it is focused, opens a TComboWindow
// from which an entry can be chosen.
//
// Palette layout: 1=Normal text 2=Focused text 3=Arrow
class TComboBox : public TView {
public:
    TComboBox(const TRect& bounds, TComboItem* aItems, short aFocused = 0) noexcept;
    ~TComboBox();

    void draw() override;
    TPalette& getPalette() const override;
    void handleEvent(TEvent& event) override;
    virtual TComboWindow* initComboWindow(const TRect& bounds);

    virtual void focusItem(short item) noexcept;
    void setState(ushort aState, Boolean enable) override;
    void newList(TComboItem* aItems, short aFocused = 0) noexcept;

    short focused;
    const char* text;
    ulong value;

protected:
    TComboItem* items;
    short numItems;
};
