#include "SettingsDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TSItem
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

TInputLine* addField(TDialog* dlg, int y, const char* label,
                      const std::string& initialValue, int maxLen) {
    dlg->insert(new TStaticText(TRect(2, y, 24, y + 1), label));
    auto* input = new TInputLine(TRect(24, y, 50, y + 1), maxLen);

    // setData() does memcpy(data, rec, maxLen) on the buffer passed in:
    // it must be at least maxLen readable bytes long, so no direct
    // c_str() of a shorter string.
    std::vector<char> buf(maxLen + 1, 0);
    std::snprintf(buf.data(), buf.size(), "%s", initialValue.c_str());
    input->setData(buf.data());

    dlg->insert(input);
    return input;
}

} // namespace

TDialog* createSettingsDialog(const AppSettings& current, const SessionLimits& sessionLimits,
                               SettingsDialogFields& fields) {
    TRect r(0, 0, 60, 23);
    auto* dlg = new TDialog(r, tr(Str::DialogTitleSettings));
    dlg->options |= ofCentered;

    fields.refreshInterval = addField(dlg, 2, tr(Str::LabelRefreshSeconds),
        std::to_string(current.refreshIntervalSeconds), 10);
    fields.host = addField(dlg, 4, tr(Str::LabelHost), current.host, 128);
    fields.port = addField(dlg, 6, tr(Str::LabelPort), std::to_string(current.port), 10);
    fields.user = addField(dlg, 8, tr(Str::LabelUser), current.user, 128);
    fields.password = addField(dlg, 10, tr(Str::LabelPassword), current.password, 128);

    dlg->insert(new TStaticText(TRect(2, 12, 24, 13), tr(Str::LabelLanguage)));
    fields.language = new TRadioButtons(TRect(24, 12, 50, 14),
        new TSItem(tr(Str::LanguageEnglish),
        new TSItem(tr(Str::LanguageItalian), nullptr)));
    ushort selectedLanguage = static_cast<ushort>(current.language);
    fields.language->setData(&selectedLanguage);
    dlg->insert(fields.language);

    // --- Global (session-wide) speed limits ---
    dlg->insert(new TStaticText(TRect(2, 16, 50, 17), tr(Str::LabelGlobalSpeedSection)));

    fields.globalLimitCheckboxes = new TCheckBoxes(TRect(2, 17, 26, 19),
        new TSItem(tr(Str::CheckGlobalLimitDownload),
        new TSItem(tr(Str::CheckGlobalLimitUpload), nullptr)));
    ushort globalChecked = (sessionLimits.downloadLimited ? 0x01 : 0) |
                           (sessionLimits.uploadLimited ? 0x02 : 0);
    fields.globalLimitCheckboxes->setData(&globalChecked);
    dlg->insert(fields.globalLimitCheckboxes);

    fields.globalDownloadLimit = new TInputLine(TRect(28, 17, 38, 18), 8);
    std::vector<char> downBuf(9, 0);
    std::snprintf(downBuf.data(), downBuf.size(), "%d", sessionLimits.downloadLimit);
    fields.globalDownloadLimit->setData(downBuf.data());
    dlg->insert(fields.globalDownloadLimit);
    dlg->insert(new TStaticText(TRect(39, 17, 44, 18), tr(Str::UnitKBs)));

    fields.globalUploadLimit = new TInputLine(TRect(28, 18, 38, 19), 8);
    std::vector<char> upBuf(9, 0);
    std::snprintf(upBuf.data(), upBuf.size(), "%d", sessionLimits.uploadLimit);
    fields.globalUploadLimit->setData(upBuf.data());
    dlg->insert(fields.globalUploadLimit);
    dlg->insert(new TStaticText(TRect(39, 18, 44, 19), tr(Str::UnitKBs)));

    dlg->insert(new TButton(TRect(20, 20, 30, 22), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(32, 20, 42, 22), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

AppSettings settingsDialogResult(const SettingsDialogFields& fields, const AppSettings& current) {
    AppSettings result = current;
    char buf[256];

    if (fields.refreshInterval) {
        fields.refreshInterval->getData(buf);
        int v = std::atoi(buf);
        if (v > 0) result.refreshIntervalSeconds = v;
    }
    if (fields.host) {
        fields.host->getData(buf);
        result.host = buf;
    }
    if (fields.port) {
        fields.port->getData(buf);
        int v = std::atoi(buf);
        if (v > 0) result.port = v;
    }
    if (fields.user) {
        fields.user->getData(buf);
        result.user = buf;
    }
    if (fields.password) {
        fields.password->getData(buf);
        result.password = buf;
    }
    if (fields.language) {
        ushort sel = 0;
        fields.language->getData(&sel);
        result.language = static_cast<Language>(sel);
    }

    return result;
}

SessionLimits settingsDialogSessionLimits(const SettingsDialogFields& fields) {
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
    return limits;
}
