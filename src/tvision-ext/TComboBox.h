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

#include <string>
#include <vector>

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

// Broadcast whenever the shown text actually changes — picking a
// different item from the dropdown, "[+]"/"[-]" (editable mode — they
// end up focusing a different item themselves), or, also in editable
// mode, directly typing/editing the text. Not fired for every open/
// close of the popup, or for cursor movement alone (Left/Right/Home/
// End) with no actual content change. cmComboBoxSelectionChanged
// itself isn't declared here: the fork this was copied from added it
// directly to views.h's own command enum (unconditionally, not gated
// behind any Uses_TComboBox guard — see cmdcodes.h/views.h), so it's
// already available from <tvision/tv.h> without needing anything
// specific to this file included first.

// Broadcast when "[+]"/"[-]" (editable mode — see below) actually add
// or remove a list entry — not fired for the no-op cases (adding text
// that already matches an entry, or "[-]" with nothing matching the
// current text; see addCurrentValue()/removeCurrentValue()'s own
// comments). Unlike cmComboBoxSelectionChanged, these two are this
// project's own addition, not something the tvision fork already
// provides — picked the next two free values after 59 in the same
// enum's own numbering (see views.h), rather than reusing anything
// from cmUserBase upward, which is reserved for application-level
// commands, not widget-level ones like this.
constexpr ushort cmComboBoxItemAdded = 60;
constexpr ushort cmComboBoxItemRemoved = 61;

// A single-line, single-selection drop-down combo box. Clicking it, or
// pressing Space/Enter/Down while it is focused, opens a TComboWindow
// from which an entry can be chosen.
//
// Palette layout: 1=Normal text 2=Focused text 3=Arrow
//
// --- Editable mode (setEditable()) ---
// Off by default (existing callers, e.g. LanguageComboBox, are
// unaffected). When on, the box can also be typed into directly rather
// than only choosing from the dropdown, and shows "[+]"/"[-]" buttons
// to its left of the dropdown arrow: "[+]" adds whatever's currently
// shown (typed fresh, or picked from the dropdown) as a new list entry
// if it isn't one already; "[-]" removes the list entry matching what's
// currently shown. allValues() reads back every entry currently in the
// list, in order — meant for a caller that persists the list somewhere
// (a config file, say) after the user has edited it via [+]/[-].
//
// Deliberately simple rather than a full TInputLine port: single-line
// ASCII only (no UTF-8-aware cursor movement — see TGridView's own
// skipLeadingUtf8() for what that would take), no horizontal scrolling
// when the typed text is wider than the box (it's just clipped), no
// text selection/clipboard. Enough for what this was built for so far
// (typing a short name), not a general-purpose editable-combo widget
// yet — worth revisiting if a future use needs any of the above.
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

    void setEditable(bool editable);
    bool isEditable() const { return editable_; }

    // What's currently shown in the box. In editable mode, this is
    // whatever's been typed or picked from the dropdown (which fills
    // this in the same way typing it would) — not necessarily an
    // existing list entry until "[+]" is used. In non-editable mode
    // this just mirrors `text` (the focused item's own text).
    std::string editText() const;
    void setEditText(const std::string& newText);

    // Every entry currently in the list, in list order.
    std::vector<std::string> allValues() const;

    // Adds editText() as a new list entry (a no-op if empty, or already
    // present — focuses the existing match instead of duplicating it).
    // Bound to the "[+]" button; also callable directly by a caller
    // that wants to ensure the currently-shown text is a real list
    // entry without requiring the user to click "[+]" themselves first
    // (e.g. a "confirm and persist" action elsewhere that types a new
    // name and saves it in one step).
    void addCurrentValue();

    // The name that was just added or removed by "[+]"/"[-]" — valid
    // (and meaningful) only while handling the matching
    // cmComboBoxItemAdded/cmComboBoxItemRemoved broadcast; a caller
    // reads this from within its own handleEvent() override right after
    // TView::handleEvent() (or the dialog's own base handleEvent())
    // delivers that broadcast to it. Not cleared afterward, so reading
    // it at any other time just returns whatever the last add/remove
    // happened to be, which usually isn't meaningful.
    const std::string& lastChangedValue() const { return lastChangedValue_; }

protected:
    TComboItem* items;
    short numItems;

private:
    bool editable_ = false;
    std::string editBuf_;
    int cursorPos_ = 0;
    std::string lastChangedValue_;

    // Opens the dropdown popup and applies the result — the same logic
    // handleEvent() always ran inline, now shared between the
    // non-editable path (Space/Enter/Down/click) and the editable one
    // (Down/clicking the arrow specifically) rather than duplicated.
    void openDropdown();
    // Removes the list entry matching editText(), if any, and focuses
    // whatever entry (if any) ends up nearest afterward. Bound to the
    // "[-]" button.
    void removeCurrentValue();

    // Screen-relative x positions of the "[+]"/"[-]" buttons and the
    // dropdown arrow, given the view's current width — shared by
    // draw() (to paint them) and handleEvent() (to hit-test clicks on
    // them), so the two can never disagree about where a button
    // actually is. Only meaningful when editable_ (non-editable mode
    // only ever has the arrow, positioned as it always was).
    struct ButtonLayout { int plusX, minusX, arrowX, textWidth; };
    ButtonLayout computeButtonLayout() const;
};
