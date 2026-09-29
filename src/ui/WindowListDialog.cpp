#include "WindowListDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TScrollBar
#define Uses_TEvent
#include <tvision/tv.h>

#include <cstdio>
#include <cstring>

WindowListViewer::WindowListViewer(const TRect& r, TScrollBar* vScrollBar,
                                    std::vector<TWindow*> windows)
    : TListViewer(r, 1, nullptr, vScrollBar), windows_(std::move(windows)) {
    setRange((short)windows_.size());
}

void WindowListViewer::getText(char* dest, short item, short maxLen) {
    if (item < 0 || item >= (int)windows_.size()) {
        dest[0] = '\0';
        return;
    }
    const char* title = windows_[item]->title;
    std::snprintf(dest, maxLen, "%s", title ? title : "");
}

TWindow* WindowListViewer::selectedWindow() const {
    if (focused < 0 || focused >= (int)windows_.size()) return nullptr;
    return windows_[focused];
}

namespace {

class WindowListDialogImpl : public TDialog {
public:
    WindowListDialogImpl(const TRect& r, TStringView title, WindowListViewer* viewer)
        : TWindowInit(&TDialog::initFrame), TDialog(r, title), viewer_(viewer) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        // Same mechanism used for the torrent list: TListViewer sends
        // this broadcast to its own owner on double-click.
        if (event.what == evBroadcast &&
            event.message.command == cmListItemSelected &&
            event.message.infoPtr == viewer_) {
            endModal(cmOK);
            clearEvent(event);
        }
    }

private:
    WindowListViewer* viewer_;
};

} // namespace

TDialog* createWindowListDialog(std::vector<TWindow*> windows, WindowListViewer*& viewer) {
    TRect r(0, 0, 40, 14);

    TRect listRect(2, 2, 36, 10);
    TScrollBar* vScroll = new TScrollBar(TRect(36, 2, 37, 10));
    viewer = new WindowListViewer(listRect, vScroll, std::move(windows));

    auto* dlg = new WindowListDialogImpl(r, tr(Str::DialogTitleWindowList), viewer);
    dlg->options |= ofCentered;

    dlg->insert(vScroll);
    dlg->insert(viewer);

    dlg->insert(new TButton(TRect(10, 11, 20, 13), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(22, 11, 32, 13), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}
