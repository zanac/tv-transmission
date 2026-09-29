#include "AboutDialog.h"
#include "Strings.h"
#include "../Version.h"

#define Uses_TButton
#define Uses_TStaticText
#include <tvision/tv.h>
#include <cstdio>
#include <ctime>

TDialog* createAboutDialog() {
    TRect r(0, 0, 54, 12);
    auto* dlg = new TDialog(r, tr(Str::DialogTitleAbout));
    dlg->options |= ofCentered;

    // Computed at runtime rather than hardcoded: stays correct without
    // needing a manual update every January.
    std::time_t now = std::time(nullptr);
    std::tm* localNow = std::localtime(&now);
    int year = localNow ? (localNow->tm_year + 1900) : 2026;

    dlg->insert(new TStaticText(TRect(2, 2, 52, 3), "TV Transmission"));

    char buf[128];
    std::snprintf(buf, sizeof(buf), tr(Str::LabelAboutVersion), kAppVersion);
    dlg->insert(new TStaticText(TRect(2, 4, 52, 5), buf));

    std::snprintf(buf, sizeof(buf), tr(Str::LabelAboutCopyright), year, "Vanni Brutto");
    dlg->insert(new TStaticText(TRect(2, 5, 52, 6), buf));

    dlg->insert(new TStaticText(TRect(2, 7, 52, 8),
        "https://github.com/zanac/tv-transmission"));

    dlg->insert(new TButton(TRect(22, 9, 32, 11), tr(Str::ButtonOK), cmOK, bfDefault));

    dlg->selectNext(False);
    return dlg;
}
