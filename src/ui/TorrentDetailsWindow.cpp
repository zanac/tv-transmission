#include "TorrentDetailsWindow.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TStaticText
#define Uses_TButton
#define Uses_TSItem
#define Uses_TEvent
#include <tvision/tv.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

// Local to this window: scoped to its own handleEvent, so it doesn't
// need to avoid App.h's cmXxx range (100-106) — a command value is only
// ever ambiguous within the single handleEvent that reacts to it.
constexpr ushort cmApplySpeedLimits = 200;

// NOT cmClose: TButton::press() sets event.message.infoPtr to the
// button itself (see tbutton.cpp), but TWindow::handleEvent only acts
// on cmClose when infoPtr is 0 or the window itself — a button-sent
// cmClose is silently ignored by the base class. Using a dedicated
// command and calling close() directly (below) sidesteps that check
// entirely; it's the same close() TWindow::handleEvent would have
// called anyway.
constexpr ushort cmCloseDetails = 201;

// TStaticText already copies the text internally (newStr + delete[] in
// its destructor), so passing it the temporary buffer is enough: there's
// no need (and it would leak memory) to duplicate it here.
void addLine(TWindow* win, int y, const char* text) {
    TRect r(2, y, 56, y + 1);
    win->insert(new TStaticText(r, text));
}

} // namespace

TorrentDetailsWindow::TorrentDetailsWindow(const TRect& bounds, TStringView title, short number,
                                            int torrentId, TransmissionClient& client)
    : TWindowInit(&TWindow::initFrame),
      TWindow(bounds, title, number),
      torrentId_(torrentId), client_(client) {}

TColorAttr TorrentDetailsWindow::mapColor(uchar index) {
    // Same technique as TorrentListViewer::mapColor() (see the comment
    // there): bypasses the palette/owner-chain lookup entirely, so
    // every color request — from this window's own frame as well as
    // from its TStaticText children, whose own palette resolution ends
    // up calling this via owner->mapColor() — gets the same fixed
    // color, regardless of the index tvision asked for.
    (void)index;
    return TColorAttr(0x0E); // fg=yellow(0xE), bg=black(0x0)
}

void TorrentDetailsWindow::applySpeedLimits() {
    ushort checked = 0;
    limitCheckboxes->getData(&checked);
    bool downloadLimited = (checked & 0x01) != 0;
    bool uploadLimited = (checked & 0x02) != 0;

    char buf[32];
    downloadLimitField->getData(buf);
    int downloadLimit = std::atoi(buf);
    uploadLimitField->getData(buf);
    int uploadLimit = std::atoi(buf);

    client_.setTorrentSpeedLimits(torrentId_, downloadLimited, downloadLimit,
                                   uploadLimited, uploadLimit);
}

void TorrentDetailsWindow::handleEvent(TEvent& event) {
    TWindow::handleEvent(event);
    if (event.what == evCommand && event.message.command == cmApplySpeedLimits) {
        applySpeedLimits();
        clearEvent(event);
    } else if (event.what == evCommand && event.message.command == cmCloseDetails) {
        close();
        clearEvent(event);
    }
}

TWindow* createTorrentDetailsWindow(const Torrent& t, TransmissionClient& client) {
    TRect r(0, 0, 58, 19);
    auto* win = new TorrentDetailsWindow(r, tr(Str::WindowTitleDetails), wnNoNumber,
                                          t.id, client);
    win->options |= ofCentered;

    char buf[256];

    std::snprintf(buf, sizeof(buf), tr(Str::LabelName), t.name.c_str());
    addLine(win, 2, buf);

    std::string sizeStr = formatSize(t.sizeBytes);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelSize), sizeStr.c_str());
    addLine(win, 3, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelCompleted), t.percentDone * 100.0);
    addLine(win, 4, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelDownload), t.rateDownload / 1024.0);
    addLine(win, 5, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelUpload), t.rateUpload / 1024.0);
    addLine(win, 6, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelStatus), trTorrentStatus(t.status));
    addLine(win, 7, buf);

    std::string addedStr = formatUnixTimestamp(t.addedDate);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelAdded), addedStr.c_str());
    addLine(win, 8, buf);

    if (!t.errorString.empty()) {
        std::snprintf(buf, sizeof(buf), tr(Str::LabelError), t.errorString.c_str());
        addLine(win, 9, buf);
    }

    std::snprintf(buf, sizeof(buf), tr(Str::LabelId), t.id);
    addLine(win, 10, buf);

    // --- Per-torrent speed limit override ---
    addLine(win, 12, tr(Str::LabelSpeedLimitSection));

    win->limitCheckboxes = new TCheckBoxes(TRect(2, 13, 26, 15),
        new TSItem(tr(Str::CheckLimitDownload),
        new TSItem(tr(Str::CheckLimitUpload), nullptr)));
    ushort checked = (t.downloadLimited ? 0x01 : 0) | (t.uploadLimited ? 0x02 : 0);
    win->limitCheckboxes->setData(&checked);
    win->insert(win->limitCheckboxes);

    win->downloadLimitField = new TInputLine(TRect(28, 13, 38, 14), 8);
    std::vector<char> downloadBuf(9, 0);
    std::snprintf(downloadBuf.data(), downloadBuf.size(), "%d", t.downloadLimit);
    win->downloadLimitField->setData(downloadBuf.data());
    win->insert(win->downloadLimitField);
    win->insert(new TStaticText(TRect(39, 13, 44, 14), tr(Str::UnitKBs)));

    win->uploadLimitField = new TInputLine(TRect(28, 14, 38, 15), 8);
    std::vector<char> uploadBuf(9, 0);
    std::snprintf(uploadBuf.data(), uploadBuf.size(), "%d", t.uploadLimit);
    win->uploadLimitField->setData(uploadBuf.data());
    win->insert(win->uploadLimitField);
    win->insert(new TStaticText(TRect(39, 14, 44, 15), tr(Str::UnitKBs)));

    win->insert(new TButton(TRect(14, 16, 24, 18), tr(Str::ButtonApply), cmApplySpeedLimits, bfDefault));
    win->insert(new TButton(TRect(28, 16, 38, 18), tr(Str::ButtonClose), cmCloseDetails, bfNormal));

    return win;
}
