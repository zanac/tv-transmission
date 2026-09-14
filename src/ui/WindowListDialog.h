#pragma once

#define Uses_TDialog
#define Uses_TListViewer
#define Uses_TWindow
#include <tvision/tv.h>
#include <vector>

// TListViewer that lists windows (by title) and lets you retrieve the
// selected one.
class WindowListViewer : public TListViewer {
public:
    WindowListViewer(const TRect& r, TScrollBar* vScrollBar,
                      std::vector<TWindow*> windows);

    void getText(char* dest, short item, short maxLen) override;
    TWindow* selectedWindow() const;

private:
    std::vector<TWindow*> windows_;
};

// Creates the "Window list" dialog with every window currently open on
// the desktop, and populates `viewer` with a pointer to it (same reason
// as the other dialogs: no reconstructing state from the TGroup).
TDialog* createWindowListDialog(std::vector<TWindow*> windows, WindowListViewer*& viewer);
