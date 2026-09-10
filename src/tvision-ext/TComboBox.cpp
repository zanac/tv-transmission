#include "TComboBox.h"

#define Uses_TKeys
#define Uses_TDrawBuffer
#define Uses_TGroup
#include <tvision/tv.h>

#include <cstring>
#include <cctype>

// ===========================================================================
// TComboViewer
// ===========================================================================

namespace {

TComboItem* nthItem(TComboItem* items, short n) noexcept {
    while (items != 0 && n > 0) {
        items = items->next;
        n--;
    }
    return items;
}

} // namespace

TComboViewer::TComboViewer(const TRect& bounds, TScrollBar* aVScrollBar,
                            TComboItem* aItems, short aFocused) noexcept :
    TListViewer(bounds, 1, 0, aVScrollBar),
    items(aItems)
{
    short count = 0;
    for (TComboItem* p = items; p != 0; p = p->next) count++;
    setRange(count);
    if (range > 0) focusItem(aFocused < range ? aFocused : short(range - 1));
}

TPalette& TComboViewer::getPalette() const {
    static const char cpComboViewer[] = "\x06\x06\x07\x06\x06";
    static TPalette palette(cpComboViewer, sizeof(cpComboViewer) - 1);
    return palette;
}

void TComboViewer::getText(char* dest, short item, short maxChars) {
    TComboItem* p = nthItem(items, item);
    if (p != 0) {
        std::strncpy(dest, p->text, maxChars);
        dest[maxChars] = EOS;
    } else {
        *dest = EOS;
    }
}

ulong TComboViewer::getValue(short item) noexcept {
    TComboItem* p = nthItem(items, item);
    return p != 0 ? p->value : 0;
}

void TComboViewer::handleEvent(TEvent& event) {
    if (event.what == evKeyDown && event.keyDown.keyCode == kbEnter) {
        endModal(cmOK);
        clearEvent(event);
    } else if ((event.what == evKeyDown && event.keyDown.keyCode == kbEsc) ||
               (event.what == evCommand && event.message.command == cmCancel)) {
        endModal(cmCancel);
        clearEvent(event);
    } else if (event.what == evMouseDown) {
        // A single click on an entry both picks it and closes the popup —
        // the same behavior as a native combo box's drop-down list.
        // Mouse events only reach here when they land inside this view
        // (the scrollbar is a separate sibling view with its own
        // handling), so there is no "click outside" case to guard here —
        // TComboWindow::handleEvent already closes the popup with
        // cmCancel for clicks outside it.
        TListViewer::handleEvent(event); // updates focused to the clicked row
        endModal(cmOK);
        clearEvent(event);
    } else if (event.what == evKeyDown && event.keyDown.charScan.charCode != ' ' &&
               event.keyDown.charScan.charCode != '\0') {
        // Incremental search: jump to the next entry whose text starts
        // with the typed character (case-insensitive), wrapping around.
        char typed = event.keyDown.charScan.charCode;
        if (range > 0) {
            char buf[256];
            for (short i = 1; i <= range; i++) {
                short candidate = (focused + i) % range;
                getText(buf, candidate, 255);
                if (buf[0] != EOS && std::toupper((uchar)buf[0]) == std::toupper((uchar)typed)) {
                    focusItemNum(candidate);
                    drawView();
                    break;
                }
            }
        }
        clearEvent(event);
    } else {
        TListViewer::handleEvent(event);
    }
}

int TComboViewer::itemWidth() noexcept {
    int width = 0;
    char buf[256];
    for (short i = 0; i < range; i++) {
        getText(buf, i, 255);
        int w = strwidth(buf);
        if (w > width) width = w;
    }
    return width;
}

// ===========================================================================
// TComboWindow
// ===========================================================================

TComboWindow::TComboWindow(const TRect& bounds, TComboItem* aItems, short aFocused) noexcept :
    TWindowInit(&TComboWindow::initFrame),
    TWindow(bounds, 0, wnNoNumber)
{
    flags = wfClose;
    TRect r = getExtent();
    r.grow(-1, -1);
    viewer = new TComboViewer(r, standardScrollBar(sbVertical | sbHandleKeyboard), aItems, aFocused);
    insert(viewer);
}

TPalette& TComboWindow::getPalette() const {
    static const char cpComboWindow[] = "\x13\x13\x15\x18\x17\x13\x14";
    static TPalette palette(cpComboWindow, sizeof(cpComboWindow) - 1);
    return palette;
}

short TComboWindow::getSelection() {
    return viewer->focused;
}

void TComboWindow::handleEvent(TEvent& event) {
    TWindow::handleEvent(event);
    if (event.what == evMouseDown && !mouseInView(event.mouse.where)) {
        endModal(cmCancel);
        clearEvent(event);
    }
}

// ===========================================================================
// TComboBox
// ===========================================================================

TComboBox::TComboBox(const TRect& bounds, TComboItem* aItems, short aFocused) noexcept :
    TView(bounds),
    focused(0),
    text(0),
    value(0),
    items(aItems),
    numItems(0)
{
    options |= ofSelectable | ofFirstClick;
    eventMask |= evBroadcast;

    for (TComboItem* p = items; p != 0; p = p->next) numItems++;

    if (numItems > 0) {
        if (aFocused < 0) aFocused = 0;
        else if (aFocused >= numItems) aFocused = numItems - 1;
        focused = aFocused;
        TComboItem* p = items;
        for (short i = 0; i < focused && p != 0; i++) p = p->next;
        if (p != 0) {
            text = p->text;
            value = p->value;
        }
    }
}

TComboBox::~TComboBox() {
    TComboItem* p = items;
    while (p != 0) {
        TComboItem* n = p->next;
        delete p;
        p = n;
    }
}

TComboBox::ButtonLayout TComboBox::computeButtonLayout() const {
    // From the right edge inward: arrow (2 cells, matching the
    // non-editable box's own size.x-2 positioning), a space, "[-]" (3
    // cells), a space, "[+]" (3 cells) — the text area is whatever's
    // left, starting at x=1 (matching the non-editable box's own
    // moveStr(1, ...) convention, leaving a 1-column left margin).
    ButtonLayout bl;
    bl.arrowX = size.x - 2;
    bl.minusX = bl.arrowX - 1 - 3;
    bl.plusX = bl.minusX - 1 - 3;
    bl.textWidth = bl.plusX - 1 - 1; // 1 for the left margin, 1 for the space before "[+]"
    if (bl.textWidth < 0) bl.textWidth = 0;
    return bl;
}

void TComboBox::draw() {
    TDrawBuffer b;
    TColorAttr color = mapColor((state & sfFocused) != 0 ? 2 : 1);

    b.moveChar(0, ' ', color, size.x);

    if (editable_) {
        ButtonLayout bl = computeButtonLayout();
        if (bl.textWidth > 0 && !editBuf_.empty()) {
            // Simple clip, not a scrolling viewport — see this class's
            // own header comment on why that's good enough for now.
            std::string shown = editBuf_.substr(0, (size_t)bl.textWidth);
            b.moveStr(1, shown.c_str(), color, bl.textWidth);
        }
        if (bl.plusX >= 0) b.moveStr(bl.plusX, "[+]", mapColor(3));
        if (bl.minusX >= 0) b.moveStr(bl.minusX, "[-]", mapColor(3));
        if (bl.arrowX >= 0) b.moveChar(bl.arrowX, '\x1F', mapColor(3), 1);
    } else {
        if (text != 0 && size.x > 3) b.moveStr(1, text, color, size.x - 3);
        if (size.x > 1) b.moveChar(size.x - 2, '\x1F', mapColor(3), 1);
    }

    writeLine(0, 0, size.x, 1, b);

    if (editable_ && (state & sfFocused) != 0) {
        ButtonLayout bl = computeButtonLayout();
        int cx = 1 + cursorPos_;
        if (cx > bl.textWidth) cx = bl.textWidth; // clamp: cursor past the clipped edge stays visible at the edge
        setCursor(cx, 0);
        showCursor();
    } else {
        hideCursor();
    }
}

TPalette& TComboBox::getPalette() const {
    static const char cpComboBox[] = "\x13\x14\x16";
    static TPalette palette(cpComboBox, sizeof(cpComboBox) - 1);
    return palette;
}

void TComboBox::focusItem(short item) noexcept {
    if (numItems == 0) return;
    if (item < 0) item = 0;
    else if (item >= numItems) item = numItems - 1;
    focused = item;

    TComboItem* p = items;
    for (short i = 0; i < item && p != 0; i++) p = p->next;
    if (p != 0) {
        text = p->text;
        value = p->value;
    }
    // Keeps the edit buffer showing whatever's actually focused — every
    // caller that changes `focused` goes through here (the dropdown
    // popup's own selection, "[+]"/"[-]", newList()), so this is the
    // one place that needs to know about editable_ rather than each of
    // them syncing it themselves.
    if (editable_) {
        editBuf_ = text ? text : "";
        cursorPos_ = (int)editBuf_.size();
    }
    drawView();
    // Broadcasts whenever the shown value changes as a result of
    // focusing a different item — covers picking from the dropdown,
    // "[+]" (focuses the newly-added entry), "[-]" (focuses whatever's
    // left), and newList() uniformly, in this one place, rather than
    // needing each call site to remember to fire it itself (this class
    // already had one bug from a similar "each caller has to remember"
    // assumption — see its own header comment on editBuf_ syncing).
    // Safe even before this view has an owner (e.g. mid-construction):
    // message() itself no-ops on a null receiver (see misc.cpp).
    message(owner, evBroadcast, cmComboBoxSelectionChanged, this);
}

void TComboBox::newList(TComboItem* aItems, short aFocused) noexcept {
    TComboItem* p = items;
    while (p != 0) {
        TComboItem* n = p->next;
        delete p;
        p = n;
    }

    items = aItems;
    numItems = 0;
    for (p = items; p != 0; p = p->next) numItems++;

    text = 0;
    value = 0;
    focused = 0;
    if (numItems > 0) {
        focusItem(aFocused); // syncs editBuf_ itself when editable_ — see its own comment
    } else {
        if (editable_) { editBuf_.clear(); cursorPos_ = 0; }
        drawView();
    }
}

TComboWindow* TComboBox::initComboWindow(const TRect& bounds) {
    return new TComboWindow(bounds, items, focused);
}

void TComboBox::setState(ushort aState, Boolean enable) {
    TView::setState(aState, enable);
    // TView::setState() does not repaint on sfFocused/sfSelected changes
    // by itself (same reason TCluster and TButton override this too) —
    // without this, the box would keep showing whichever color it was
    // last drawn with even after focus moves elsewhere.
    if (aState & (sfSelected | sfFocused)) drawView();
}

void TComboBox::openDropdown() {
    if (numItems == 0) return;
    TRect r = getBounds();
    r.a.y++;
    short rows = numItems < 8 ? numItems : 8;
    r.b.y = r.a.y + rows + 2;
    TRect limits = owner->getExtent();
    r.intersect(limits);

    TComboWindow* win = initComboWindow(r);
    if (win != 0) {
        ushort c = owner->execView(win);
        if (c == cmOK) {
            short sel = win->getSelection();
            // In editable mode, re-focus (and so re-sync editBuf_, and
            // re-broadcast — both now handled by focusItem() itself,
            // see its own comment) even if the selection landed back on
            // the same item as before: the box may currently be showing
            // something the user typed rather than that item's own
            // text, and picking it from the dropdown again should
            // still restore it.
            if (sel != focused || editable_) {
                focusItem(sel);
            }
        }
        destroy(win);
    }
    drawView();
}

void TComboBox::addCurrentValue() {
    if (editBuf_.empty()) return;
    short idx = 0;
    for (TComboItem* p = items; p != 0; p = p->next, idx++) {
        if (editBuf_ == p->text) {
            focusItem(idx); // already there — just select it, no duplicate,
            return;         // and no broadcast: nothing actually changed
        }
    }
    lastChangedValue_ = editBuf_; // captured before anything else touches editBuf_
    TComboItem* newItem = new TComboItem(editBuf_.c_str(), 0, nullptr);
    if (items == nullptr) {
        items = newItem;
    } else {
        TComboItem* p = items;
        while (p->next != nullptr) p = p->next;
        p->next = newItem;
    }
    numItems++;
    focusItem((short)(numItems - 1)); // the newly-appended entry
    message(owner, evBroadcast, cmComboBoxItemAdded, this);
}

void TComboBox::removeCurrentValue() {
    if (editBuf_.empty() || items == nullptr) return;
    TComboItem* prev = nullptr;
    TComboItem* p = items;
    short idx = 0;
    while (p != nullptr && editBuf_ != p->text) {
        prev = p;
        p = p->next;
        idx++;
    }
    if (p == nullptr) return; // current text doesn't match any entry — no broadcast

    lastChangedValue_ = p->text; // captured before the entry is freed below

    if (prev == nullptr) items = p->next;
    else prev->next = p->next;
    delete p;
    numItems--;

    if (numItems == 0) {
        focused = 0;
        text = nullptr;
        value = 0;
        editBuf_.clear();
        cursorPos_ = 0;
        drawView();
    } else {
        // Focuses whatever entry now sits at the removed one's old
        // position (or the new last entry, if it was the last one) —
        // syncs editBuf_ via focusItem() itself.
        focusItem(idx < numItems ? idx : (short)(numItems - 1));
    }
    message(owner, evBroadcast, cmComboBoxItemRemoved, this);
}

std::string TComboBox::editText() const {
    return editable_ ? editBuf_ : (text ? text : "");
}

void TComboBox::setEditText(const std::string& newText) {
    editBuf_ = newText;
    cursorPos_ = (int)editBuf_.size();
    drawView();
}

std::vector<std::string> TComboBox::allValues() const {
    std::vector<std::string> result;
    for (TComboItem* p = items; p != 0; p = p->next) result.push_back(p->text);
    return result;
}

void TComboBox::setEditable(bool editable) {
    editable_ = editable;
    if (editable_) {
        editBuf_ = text ? text : "";
        cursorPos_ = (int)editBuf_.size();
    }
    drawView();
}

void TComboBox::handleEvent(TEvent& event) {
    TView::handleEvent(event);

    if (editable_) {
        if (event.what == evMouseDown) {
            ButtonLayout bl = computeButtonLayout();
            TPoint local = makeLocal(event.mouse.where);
            if (local.x >= bl.plusX && local.x < bl.plusX + 3) {
                addCurrentValue();
                clearEvent(event);
            } else if (local.x >= bl.minusX && local.x < bl.minusX + 3) {
                removeCurrentValue();
                clearEvent(event);
            } else if (numItems > 0 && local.x >= bl.arrowX && local.x < bl.arrowX + 2) {
                openDropdown();
                clearEvent(event);
            } else if (local.x >= 0 && local.x < bl.textWidth + 1) {
                // Simple: click anywhere in the text area moves the
                // cursor to the end, rather than to the clicked
                // column — matches this class's own "no scrolling
                // viewport, no real cursor math" simplification (see
                // its header comment).
                cursorPos_ = (int)editBuf_.size();
                drawView();
                clearEvent(event);
            }
            return;
        }

        if (event.what == evKeyDown) {
            ushort key = event.keyDown.keyCode;
            if (numItems > 0 && ctrlToArrow(key) == kbDown) {
                openDropdown();
                clearEvent(event);
            } else if (key == kbLeft) {
                if (cursorPos_ > 0) { cursorPos_--; drawView(); }
                clearEvent(event);
            } else if (key == kbRight) {
                if (cursorPos_ < (int)editBuf_.size()) { cursorPos_++; drawView(); }
                clearEvent(event);
            } else if (key == kbHome) {
                cursorPos_ = 0;
                drawView();
                clearEvent(event);
            } else if (key == kbEnd) {
                cursorPos_ = (int)editBuf_.size();
                drawView();
                clearEvent(event);
            } else if (key == kbBack) {
                if (cursorPos_ > 0) {
                    editBuf_.erase(cursorPos_ - 1, 1);
                    cursorPos_--;
                    drawView();
                    // Content actually changed by typing, not by
                    // focusing a different item — focusItem() doesn't
                    // run this path, so it has to fire here instead.
                    // Same event as focusItem()'s own, deliberately: the
                    // caller only cares that the shown text is now
                    // different, not how it got that way.
                    message(owner, evBroadcast, cmComboBoxSelectionChanged, this);
                }
                clearEvent(event);
            } else if (key == kbDel) {
                if (cursorPos_ < (int)editBuf_.size()) {
                    editBuf_.erase(cursorPos_, 1);
                    drawView();
                    message(owner, evBroadcast, cmComboBoxSelectionChanged, this);
                }
                clearEvent(event);
            } else {
                char ch = event.keyDown.charScan.charCode;
                // Printable ASCII only (see this class's own header
                // comment on why) — everything else (Enter, Tab, Esc,
                // F-keys, ...) is deliberately left uncleared here, so
                // it still reaches whatever the dialog's own handling
                // does with it (e.g. Enter triggering the default
                // button).
                if (ch >= 32 && ch < 127 && editBuf_.size() < 128) {
                    editBuf_.insert(editBuf_.begin() + cursorPos_, ch);
                    cursorPos_++;
                    drawView();
                    message(owner, evBroadcast, cmComboBoxSelectionChanged, this);
                    clearEvent(event);
                }
            }
        }
        return;
    }

    // --- Non-editable: unchanged original behavior. ---
    if (numItems > 0 &&
        (event.what == evMouseDown ||
         (event.what == evKeyDown &&
          (event.keyDown.charScan.charCode == ' ' ||
           event.keyDown.keyCode == kbEnter ||
           ctrlToArrow(event.keyDown.keyCode) == kbDown)))) {
        openDropdown();
        clearEvent(event);
    }
}
