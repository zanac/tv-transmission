#include "ServerSettingsDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TSItem
#define Uses_TValidator
#define Uses_TRangeValidator
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

// Shared by both the global and the alt-speed sections: an input line
// pre-filled with `value`, validated to a sane KB/s range.
TInputLine* addLimitField(TDialog* dlg, TRect r, int value) {
    auto* input = new TInputLine(r, 8);
    std::vector<char> buf(9, 0);
    std::snprintf(buf.data(), buf.size(), "%d", value);
    input->setData(buf.data());
    input->setValidator(new TRangeValidator(0, 1000000)); // KB/s, ~1GB/s cap
    dlg->insert(input);
    return input;
}

} // namespace

TDialog* createServerSettingsDialog(const SessionLimits& sessionLimits,
                                     ServerSettingsDialogFields& fields) {
    TRect r(0, 0, 60, 19);
    auto* dlg = new TDialog(r, tr(Str::DialogTitleServerSettings));
    dlg->options |= ofCentered;

    // --- Global (session-wide) speed limits ---
    dlg->insert(new TStaticText(TRect(2, 2, 56, 3), tr(Str::LabelGlobalSpeedSection)));

    fields.globalLimitCheckboxes = new TCheckBoxes(TRect(2, 3, 26, 5),
        new TSItem(tr(Str::CheckGlobalLimitDownload),
        new TSItem(tr(Str::CheckGlobalLimitUpload), nullptr)));
    ushort globalChecked = (sessionLimits.downloadLimited ? 0x01 : 0) |
                           (sessionLimits.uploadLimited ? 0x02 : 0);
    fields.globalLimitCheckboxes->setData(&globalChecked);
    dlg->insert(fields.globalLimitCheckboxes);

    fields.globalDownloadLimit = addLimitField(dlg, TRect(28, 3, 38, 4), sessionLimits.downloadLimit);
    dlg->insert(new TStaticText(TRect(39, 3, 44, 4), tr(Str::UnitKBs)));

    fields.globalUploadLimit = addLimitField(dlg, TRect(28, 4, 38, 5), sessionLimits.uploadLimit);
    dlg->insert(new TStaticText(TRect(39, 4, 44, 5), tr(Str::UnitKBs)));

    // --- "Speed Limit" (alt-speed / turtle) mode ---
    dlg->insert(new TStaticText(TRect(2, 7, 56, 8), tr(Str::LabelAltSpeedSection)));

    fields.altSpeedEnabledCheckbox = new TCheckBoxes(TRect(2, 8, 40, 9),
        new TSItem(tr(Str::CheckAltSpeedEnabled), nullptr));
    ushort altSpeedChecked = sessionLimits.altSpeedEnabled ? 0x01 : 0;
    fields.altSpeedEnabledCheckbox->setData(&altSpeedChecked);
    dlg->insert(fields.altSpeedEnabledCheckbox);

    // 2 rows tall: the translated text embeds a '\n' at a natural break
    // point for each language (see Strings.cpp) rather than relying on
    // TStaticText to wrap on its own.
    dlg->insert(new TStaticText(TRect(2, 9, 56, 11), tr(Str::LabelAltSpeedDescription)));

    dlg->insert(new TStaticText(TRect(2, 12, 24, 13), tr(Str::LabelAltSpeedDownload)));
    fields.altSpeedDownloadLimit = addLimitField(dlg, TRect(28, 12, 38, 13), sessionLimits.altSpeedDown);
    dlg->insert(new TStaticText(TRect(39, 12, 44, 13), tr(Str::UnitKBs)));

    dlg->insert(new TStaticText(TRect(2, 13, 24, 14), tr(Str::LabelAltSpeedUpload)));
    fields.altSpeedUploadLimit = addLimitField(dlg, TRect(28, 13, 38, 14), sessionLimits.altSpeedUp);
    dlg->insert(new TStaticText(TRect(39, 13, 44, 14), tr(Str::UnitKBs)));

    dlg->insert(new TButton(TRect(20, 16, 30, 18), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(32, 16, 42, 18), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

SessionLimits serverSettingsDialogResult(const ServerSettingsDialogFields& fields) {
    SessionLimits limits;
    if (fields.globalLimitCheckboxes) {
        ushort checked = 0;
        fields.globalLimitCheckboxes->getData(&checked);
        limits.downloadLimited = (checked & 0x01) != 0;
        limits.uploadLimited = (checked & 0x02) != 0;
    }
    char buf[32];
    if (fields.globalDownloadLimit) {
        fields.globalDownloadLimit->getData(buf);
        limits.downloadLimit = std::atoi(buf);
    }
    if (fields.globalUploadLimit) {
        fields.globalUploadLimit->getData(buf);
        limits.uploadLimit = std::atoi(buf);
    }
    if (fields.altSpeedEnabledCheckbox) {
        ushort checked = 0;
        fields.altSpeedEnabledCheckbox->getData(&checked);
        limits.altSpeedEnabled = (checked & 0x01) != 0;
    }
    if (fields.altSpeedDownloadLimit) {
        fields.altSpeedDownloadLimit->getData(buf);
        limits.altSpeedDown = std::atoi(buf);
    }
    if (fields.altSpeedUploadLimit) {
        fields.altSpeedUploadLimit->getData(buf);
        limits.altSpeedUp = std::atoi(buf);
    }
    return limits;
}
