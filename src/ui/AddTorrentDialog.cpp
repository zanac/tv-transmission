#include "AddTorrentDialog.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TEvent
#include <tvision/tv.h>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cctype>

namespace {

// Local to this dialog: scoped to its own handleEvent, same reasoning
// as similar local command constants elsewhere in this project.
//
// 204, not a low unused-looking number: tvision's own built-in
// commands occupy most of 0-102, 301, and 500 upward (see cmClose's
// own comment on cmChangeFolder below for the full story) — this one
// was originally 3, which collides with tvision's own cmMenu. Not
// otherwise visibly broken (nothing here happens to depend on
// activating the menu bar), but not something to leave in place now
// that the same class of bug was found and fixed once already in this
// same file.
constexpr ushort cmVerifyFreeSpace = 204;
// cmChangeFolder itself is declared in AddTorrentDialog.h (its own
// comment there explains why it needs to be its own dedicated command,
// not shared with cmNo) — App.cpp's own result switch needs to see it
// too, so it can't stay local to this file the way cmVerifyFreeSpace
// above does.

// "Change..." opens a folder browser that only ever looks at THIS
// machine's own filesystem (see TFolderBrowserDialog's own doc comment
// on why RPC has no way to browse the daemon's filesystem instead) —
// meaningful only when the daemon being talked to is running on this
// same machine. For a remote daemon, browsing a folder here wouldn't
// correspond to anything on the machine that would actually use it, so
// the button is disabled outright rather than left clickable and
// misleading; "Verify" (and the destination shown) still work
// regardless, since those go through the daemon's own RPC either way,
// not this machine's filesystem.
bool isLocalHost(const std::string& host) {
    if (host == "127.0.0.1") return true;
    std::string lower = host;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return lower == "localhost";
}

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
        } else if (event.message.command == cmChangeFolder) {
            // Unlike cmOK/cmCancel/cmYes/cmNo, TDialog's own
            // handleEvent() has no built-in knowledge of this command
            // (it's this project's own, not tvision's) and won't end
            // the modal loop for it by itself — found directly, right
            // after fixing the command-id collision this button had
            // instead (see cmChangeFolder's own comment in
            // AddTorrentDialog.h): the button drew enabled, colored
            // correctly, and a click reached here (this handler), but
            // execView() itself never returned, so the caller's own
            // "if (result == cmChangeFolder)" branch in App.cpp was
            // simply never reached. endModal() here is what cmNo used
            // to provide for free.
            endModal(cmChangeFolder);
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

    // Hidden outright for a remote daemon, not just disabled — a
    // greyed-out button that's still clearly THERE invites clicking it
    // anyway to see what happens (reported directly: it still opened
    // the folder browser despite looking disabled, a separate bug now
    // fixed — see cmChangeFolder's own comment in AddTorrentDialog.h —
    // but even with that fixed, a visible-but-inert button is still a
    // button someone will eventually click and wonder why nothing
    // happened). Hiding it removes that question entirely. Set fresh
    // every time this dialog is built, not just once, since the same
    // dialog gets rebuilt for whichever server is currently focused —
    // a previous open against a local server must not leave this
    // visible for a later one against a remote one.
    //
    // enableCommand() here is unconditional, not tied to isLocalHost()
    // — visibility alone decides whether the button can be seen or
    // reached at all now, so the command itself just needs to not be
    // globally disabled (TButton's own constructor checks
    // commandEnabled() and starts sfDisabled otherwise — see
    // tbutton.cpp — and nothing else in the app ever enables this
    // command, so without this call the button stayed sfDisabled even
    // while visible for a LOCAL server, a second bug this introduced
    // on top of the first one it was meant to fix).
    dlg->enableCommand(cmChangeFolder);
    auto* changeFolderButton = new TButton(TRect(2, 2, 16, 4), tr(Str::ButtonChangeFolder), cmChangeFolder, bfNormal);
    changeFolderButton->setState(sfVisible, isLocalHost(client.getHost()) ? True : False);
    dlg->insert(changeFolderButton);
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
