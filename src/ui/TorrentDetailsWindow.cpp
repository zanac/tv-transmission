#include "TorrentDetailsWindow.h"
#include "Strings.h"

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

TWindow* createTorrentDetailsWindow(const Torrent& t) {
    TRect r(0, 0, 58, 12);
    auto* win = new TWindow(r, tr(Str::WindowTitleDetails), wnNoNumber);
    win->options |= ofCentered;

    char buf[256];

    std::snprintf(buf, sizeof(buf), tr(Str::LabelName), t.name.c_str());
    addLine(win, 2, buf);

    double mb = t.sizeBytes / (1024.0 * 1024.0);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelSize), mb);
    addLine(win, 3, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelCompleted), t.percentDone * 100.0);
    addLine(win, 4, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelDownload), t.rateDownload / 1024.0);
    addLine(win, 5, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelUpload), t.rateUpload / 1024.0);
    addLine(win, 6, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelStatus), trTorrentStatus(t.status));
    addLine(win, 7, buf);

    if (!t.errorString.empty()) {
        std::snprintf(buf, sizeof(buf), tr(Str::LabelError), t.errorString.c_str());
        addLine(win, 8, buf);
    }

    std::snprintf(buf, sizeof(buf), tr(Str::LabelId), t.id);
    addLine(win, 9, buf);

    return win;
}
