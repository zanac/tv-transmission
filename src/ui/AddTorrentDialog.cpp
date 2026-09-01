#include "AddTorrentDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TStaticText
#include <tvision/tv.h>
#include <vector>

TDialog* createAddTorrentDialog(TInputLine*& urlField) {
    TRect r(0, 0, 60, 8);
    auto* dlg = new TDialog(r, tr(Str::DialogTitleAddTorrent));
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 57, 3), tr(Str::LabelAddTorrentUrl)));

    urlField = new TInputLine(TRect(2, 3, 57, 4), 512);
    // setData() does a memcpy from the buffer passed in, for maxLen
    // bytes: nullptr here would crash. Use an empty but correctly sized
    // buffer.
    std::vector<char> emptyBuf(513, 0);
    urlField->setData(emptyBuf.data());
    dlg->insert(urlField);

    dlg->insert(new TButton(TRect(20, 5, 30, 7), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(32, 5, 42, 7), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

std::string addTorrentDialogResult(TInputLine* urlField) {
    if (!urlField) return "";
    char buf[513] = {0};
    urlField->getData(buf);
    return buf;
}
