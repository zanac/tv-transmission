#include "TorrentDetailsWindow.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TStaticText
#include <tvision/tv.h>
#include <cstdio>
#include <cstring>

namespace {

// TStaticText already copies the text internally (newStr + delete[] in
// its destructor), so passing it the temporary buffer is enough: there's
// no need (and it would leak memory) to duplicate it here.
void addLine(TWindow* win, int y, const char* text) {
    TRect r(2, y, 56, y + 1);
    win->insert(new TStaticText(r, text));
}

} // namespace

TorrentDetailsWindow::TorrentDetailsWindow(const TRect& bounds, TStringView title, short number)
    : TWindowInit(&TWindow::initFrame),
      TWindow(bounds, title, number) {}

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

TWindow* createTorrentDetailsWindow(const Torrent& t) {
    TRect r(0, 0, 58, 13);
    auto* win = new TorrentDetailsWindow(r, tr(Str::WindowTitleDetails), wnNoNumber);
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

    return win;
}
