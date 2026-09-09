#include "ConnectionDialog.h"
#include "Strings.h"
#include "PasswordInputLine.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TValidator
#define Uses_TRangeValidator
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

TInputLine* addField(TDialog* dlg, int y, const char* label,
                      const std::string& initialValue, int maxLen,
                      bool masked = false) {
    dlg->insert(new TStaticText(TRect(2, y, 24, y + 1), label));
    TInputLine* input = masked
        ? static_cast<TInputLine*>(new PasswordInputLine(TRect(24, y, 50, y + 1), maxLen))
        : new TInputLine(TRect(24, y, 50, y + 1), maxLen);

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

TDialog* createConnectionDialog(const AppSettings& current, ConnectionDialogFields& fields) {
    TRect r(0, 0, 60, 16);
    auto* dlg = new TDialog(r, tr(Str::DialogTitleConnection));
    dlg->options |= ofCentered;

    fields.refreshInterval = addField(dlg, 2, tr(Str::LabelRefreshSeconds),
        std::to_string(current.refreshIntervalSeconds), 10);
    // TRangeValidator filters non-digit keystrokes as they're typed (see
    // isValidInput() in tvision's tvalidat.cpp) and blocks confirming the
    // dialog with an out-of-range value (shows its own "Value not in the
    // range X to Y" messageBox) — real validation, not just parsing
    // whatever ends up in the field after the fact.
    fields.refreshInterval->setValidator(new TRangeValidator(1, 86400)); // up to 24h
    fields.host = addField(dlg, 4, tr(Str::LabelHost), current.host, 128);
    fields.port = addField(dlg, 6, tr(Str::LabelPort), std::to_string(current.port), 10);
    fields.port->setValidator(new TRangeValidator(1, 65535)); // valid TCP port range
    fields.user = addField(dlg, 8, tr(Str::LabelUser), current.user, 128);
    fields.password = addField(dlg, 10, tr(Str::LabelPassword), current.password, 128, /*masked=*/true);

    dlg->insert(new TStaticText(TRect(2, 12, 24, 13), tr(Str::LabelLanguage)));
    fields.language = new LanguageComboBox(TRect(24, 12, 50, 13), current.language);
    dlg->insert(fields.language);

    dlg->insert(new TButton(TRect(20, 13, 30, 15), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(32, 13, 42, 15), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

AppSettings connectionDialogResult(const ConnectionDialogFields& fields, const AppSettings& current) {
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
        result.language = fields.language->language();
    }

    return result;
}
