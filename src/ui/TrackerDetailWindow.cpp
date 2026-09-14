#include "TrackerDetailWindow.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TStaticText
#define Uses_TButton
#define Uses_TEvent
#include <tvision/tv.h>
#include <cstdio>

namespace {

// Same reasoning as TorrentDetailsWindow's cmCloseDetails: TButton::
// press() sets infoPtr to the button itself, which TWindow/TDialog's
// own cmClose handling doesn't recognize — a dedicated command plus a
// direct close() call sidesteps that.
constexpr ushort cmCloseTrackerDetail = 220;

void addLine(TWindow* win, int y, const char* text) {
    TRect r(2, y, 60, y + 1);
    win->insert(new TStaticText(r, text));
}

class TrackerDetailDialog : public TDialog {
public:
    TrackerDetailDialog(const TRect& r, TStringView title)
        : TWindowInit(&TDialog::initFrame), TDialog(r, title) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what == evCommand && event.message.command == cmCloseTrackerDetail) {
            close();
            clearEvent(event);
        }
    }
};

} // namespace

TWindow* createTrackerDetailWindow(const TrackerStat& t) {
    TRect r(0, 0, 62, 13);
    auto* win = new TrackerDetailDialog(r, tr(Str::WindowTitleTrackerDetail));
    win->options |= ofCentered;

    char buf[320];

    std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerHost), t.host.c_str());
    addLine(win, 2, buf);

    std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerTier), t.tier + 1);
    addLine(win, 3, buf);

    std::string seeders = t.seederCount < 0 ? tr(Str::ValueNotAvailable) : std::to_string(t.seederCount);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerSeeders), seeders.c_str());
    addLine(win, 4, buf);

    std::string leechers = t.leecherCount < 0 ? tr(Str::ValueNotAvailable) : std::to_string(t.leecherCount);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerLeechers), leechers.c_str());
    addLine(win, 5, buf);

    std::string downloaded = t.downloadCount < 0 ? tr(Str::ValueNotAvailable) : std::to_string(t.downloadCount);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerDownloaded), downloaded.c_str());
    addLine(win, 6, buf);

    std::string lastAnnounce = formatUnixTimestamp(t.lastAnnounceTime);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerLastAnnounce),
        lastAnnounce.empty() ? tr(Str::ValueNotAvailable) : lastAnnounce.c_str());
    addLine(win, 7, buf);

    std::string nextAnnounce = formatUnixTimestamp(t.nextAnnounceTime);
    std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerNextAnnounce),
        nextAnnounce.empty() ? tr(Str::ValueNotAvailable) : nextAnnounce.c_str());
    addLine(win, 8, buf);

    if (!t.lastAnnounceResult.empty()) {
        std::snprintf(buf, sizeof(buf), tr(Str::LabelTrackerResult), t.lastAnnounceResult.c_str());
        addLine(win, 9, buf);
    }

    win->insert(new TButton(TRect(24, 11, 36, 13), tr(Str::ButtonClose), cmCloseTrackerDetail, bfDefault));

    return win;
}
