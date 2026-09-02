#include "AddTorrentDialog.h"
#include "FileBrowserDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TEvent
#include <tvision/tv.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// Local to this dialog: scoped to its own handleEvent (same reasoning
// as the other dedicated commands in this app, e.g.
// TorrentDetailsWindow's cmApplySpeedLimits).
constexpr ushort cmBrowseForFile = 230;

class AddTorrentDialogImpl : public TDialog {
public:
    AddTorrentDialogImpl(const TRect& r, TStringView title, TInputLine* urlField)
        : TWindowInit(&TDialog::initFrame), TDialog(r, title), urlField_(urlField) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what == evCommand && event.message.command == cmBrowseForFile) {
            browseForFile();
            clearEvent(event);
        }
    }

private:
    void browseForFile() {
        // Same nested-dialog pattern as any other modal dialog in this
        // app (see App.cpp's execView(dlg) calls) — the only difference
        // is that `this` (the Add-torrent dialog itself, a TGroup) is
        // what execView() is called on here, instead of the top-level
        // TApplication, since this dialog is what needs to stay open
        // underneath while the browser runs on top of it.
        //
        // A custom browser (see FileBrowserDialog.h) rather than
        // tvision's own TFileDialog: nesting TFileDialog inside this
        // already-modal dialog rendered with wrong (jarring red)
        // colors — most likely a palette-resolution quirk specific to
        // two levels of dialog nesting, which even tvision's own
        // "tvedit" example never does (it opens TFileDialog directly
        // from the application, one level deep). Side-stepped by not
        // using TFileDialog at all here.
        const char* home = std::getenv("HOME");
        auto* browseDlg = createFileBrowserDialog(home ? home : "/");
        if (execView(browseDlg) == cmOK) {
            std::string path = fileBrowserDialogResult(browseDlg);
            std::vector<char> data(513, 0);
            std::snprintf(data.data(), data.size(), "%s", path.c_str());
            urlField_->setData(data.data());
            urlField_->drawView();
        }
        TObject::destroy(browseDlg);
    }

    TInputLine* urlField_;
};

} // namespace

TDialog* createAddTorrentDialog(TInputLine*& urlField) {
    TRect r(0, 0, 60, 8);

    urlField = new TInputLine(TRect(2, 3, 57, 4), 512);
    // setData() does a memcpy from the buffer passed in, for maxLen
    // bytes: nullptr here would crash. Use an empty but correctly sized
    // buffer.
    std::vector<char> emptyBuf(513, 0);
    urlField->setData(emptyBuf.data());

    auto* dlg = new AddTorrentDialogImpl(r, tr(Str::DialogTitleAddTorrent), urlField);
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 57, 3), tr(Str::LabelAddTorrentUrl)));
    dlg->insert(urlField);
    dlg->insert(new TButton(TRect(2, 5, 16, 7), tr(Str::ButtonBrowse), cmBrowseForFile, bfNormal));

    dlg->insert(new TButton(TRect(28, 5, 38, 7), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(40, 5, 50, 7), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

std::string addTorrentDialogResult(TInputLine* urlField) {
    if (!urlField) return "";
    char buf[513] = {0};
    urlField->getData(buf);
    return buf;
}
