#include "AddTorrentDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TStaticText
#include <tvision/tv.h>
#include <vector>
#include <cstdio>

TDialog* createAddTorrentDialog(TInputLine*& urlField, const std::string& initialValue) {
    TRect r(0, 0, 60, 8);
    auto* dlg = new TDialog(r, tr(Str::DialogTitleAddTorrent));
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 57, 3), tr(Str::LabelAddTorrentUrl)));

    urlField = new TInputLine(TRect(2, 3, 57, 4), 512);
    // setData() does a memcpy from the buffer passed in, for maxLen
    // bytes: nullptr here would crash. Use an empty (or pre-filled, see
    // `initialValue` in the header comment) but correctly sized buffer.
    std::vector<char> buf(513, 0);
    std::snprintf(buf.data(), buf.size(), "%s", initialValue.c_str());
    urlField->setData(buf.data());
    dlg->insert(urlField);

    dlg->insert(new TButton(TRect(2, 5, 16, 7), tr(Str::ButtonBrowse), cmYes, bfNormal));
    dlg->insert(new TButton(TRect(28, 5, 38, 7), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(40, 5, 50, 7), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

std::string addTorrentDialogResult(TInputLine* urlField) {
    if (!urlField) return "";
    char buf[513] = {0};
    urlField->getData(buf);
    return buf;
}
