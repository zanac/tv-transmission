#include "AddTorrentDialog.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TEvent
#include <tvision/tv.h>
#include <vector>
#include <cstdio>

namespace {

// Local to this dialog: scoped to its own handleEvent, same reasoning
// as similar local command constants elsewhere in this project.
constexpr ushort cmVerifyFreeSpace = 3;

// TStaticText with a settable body — see ServerSettingsDialog.cpp's own
// TResultLabel for why the base class doesn't have one, and why this
// is duplicated here rather than shared (a few lines, file-local, no
// home in this project for a view helper used by more than two
// unrelated dialogs — now three, but the reasoning hasn't changed).
class TResultLabel : public TStaticText {
public:
    TResultLabel(const TRect& bounds, TStringView aText) : TStaticText(bounds, aText) {}
    void setText(const std::string& s) {
        delete[] (char*)text;
        text = newStr(s.c_str());
        drawView();
    }
};

class AddTorrentDialogImpl : public TDialog {
public:
    AddTorrentDialogImpl(const TRect& bounds, TStringView title, TransmissionClient& client)
        : TWindowInit(&TDialog::initFrame), TDialog(bounds, title), client_(client) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what != evCommand) return;
        if (event.message.command == cmVerifyFreeSpace) {
            bool ok = false;
            int64_t bytes = client_.getFreeSpace(destination_, &ok);
            if (spaceLabel) spaceLabel->setText(ok ? formatSize(bytes) : tr(Str::ValueNotAvailable));
            clearEvent(event);
        }
    }

    // Truncated for display (a real path can easily be wider than this
    // dialog is) — destination_ itself, what actually gets used for
    // the free-space check and what's returned to the caller, always
    // keeps the untruncated value.
    void setDestination(const std::string& path) {
        destination_ = path;
        if (destinationLabel) destinationLabel->setText(truncateUtf8(path, 38));
    }

    TResultLabel* destinationLabel = nullptr;
    TResultLabel* spaceLabel = nullptr;
    std::string destination_;

private:
    TransmissionClient& client_;
};

} // namespace

TDialog* createAddTorrentDialog(TInputLine*& urlField, TransmissionClient& client,
                                 const std::string& initialValue,
                                 const std::string& initialDestination) {
    TRect r(0, 0, 60, 14);
    auto* dlg = new AddTorrentDialogImpl(r, tr(Str::DialogTitleAddTorrent), client);
    dlg->options |= ofCentered;

    dlg->insert(new TButton(TRect(2, 2, 16, 4), tr(Str::ButtonChangeFolder), cmNo, bfNormal));
    dlg->destinationLabel = new TResultLabel(TRect(18, 2, 57, 3), "");
    dlg->insert(dlg->destinationLabel);

    dlg->insert(new TButton(TRect(2, 4, 16, 6), tr(Str::ButtonVerify), cmVerifyFreeSpace, bfNormal));
    dlg->spaceLabel = new TResultLabel(TRect(18, 4, 57, 5), "");
    dlg->insert(dlg->spaceLabel);

    dlg->insert(new TStaticText(TRect(2, 7, 57, 8), tr(Str::LabelAddTorrentUrl)));

    urlField = new TInputLine(TRect(2, 8, 57, 9), 512);
    // setData() does a memcpy from the buffer passed in, for maxLen
    // bytes: nullptr here would crash. Use an empty (or pre-filled, see
    // `initialValue` in the header comment) but correctly sized buffer.
    std::vector<char> buf(513, 0);
    std::snprintf(buf.data(), buf.size(), "%s", initialValue.c_str());
    urlField->setData(buf.data());
    dlg->insert(urlField);

    dlg->insert(new TButton(TRect(2, 11, 16, 13), tr(Str::ButtonBrowse), cmYes, bfNormal));
    dlg->insert(new TButton(TRect(28, 11, 38, 13), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(40, 11, 50, 13), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);

    // `initialDestination` (preserved across a Browse/Change reopen —
    // see this function's own doc comment in the header) if given,
    // otherwise the server's own default download directory, fetched
    // fresh — a live RPC call, same as every other per-open fetch in
    // this app's own dialogs (Server Settings, Session Statistics).
    std::string destination = initialDestination;
    if (destination.empty()) destination = client.getDefaultDownloadDir();
    dlg->setDestination(destination);

    // Free space fetched automatically right away too, rather than
    // requiring a "Verify" click just to see a number the first time —
    // "Verify" stays for re-checking later, e.g. after some other
    // torrent finishes and frees space up while this dialog happens to
    // still be open.
    bool spaceOk = false;
    int64_t bytes = client.getFreeSpace(destination, &spaceOk);
    dlg->spaceLabel->setText(spaceOk ? formatSize(bytes) : tr(Str::ValueNotAvailable));

    return dlg;
}

std::string addTorrentDialogResult(TInputLine* urlField) {
    if (!urlField) return "";
    char buf[513] = {0};
    urlField->getData(buf);
    return buf;
}

std::string addTorrentDialogDestination(TDialog* dialog) {
    auto* impl = dynamic_cast<AddTorrentDialogImpl*>(dialog);
    return impl ? impl->destination_ : "";
}
