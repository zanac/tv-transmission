#include "CopyToServerDialog.h"
#include "Strings.h"
#include "../tvision-ext/TComboBox.h"

#define Uses_TProgram
#define Uses_TApplication
#define Uses_TDialog
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TRect
#include <tvision/tv.h>

#include <cstdio>

std::string chooseTargetServer(const std::vector<std::string>& serverNames,
                               const std::string& subject) {
    if (serverNames.empty()) return std::string();

    // Built back to front: TComboItem is a singly-linked list and the
    // combo takes ownership of the whole chain.
    TComboItem* items = nullptr;
    for (size_t i = serverNames.size(); i-- > 0;)
        items = new TComboItem(serverNames[i], (ulong)i, items);

    TRect r(0, 0, 56, 11);
    auto* dlg = new TDialog(r, tr(Str::DialogTitleCopyToServer));
    dlg->options |= ofCentered;

    // Long torrent names are cut so the label always fits one line.
    std::string shown = subject;
    const size_t maxName = 44;
    if (shown.size() > maxName) shown = shown.substr(0, maxName - 3) + "...";
    char label[256];
    std::snprintf(label, sizeof(label), tr(Str::LabelCopyToServer), shown.c_str());
    dlg->insert(new TStaticText(TRect(2, 2, 54, 4), label));

    auto* combo = new TComboBox(TRect(2, 5, 54, 6), items, 0);
    dlg->insert(combo);

    auto* add = new TButton(TRect(14, 8, 26, 10), tr(Str::ButtonAddShort), cmOK, bfDefault);
    auto* cancel = new TButton(TRect(30, 8, 42, 10), tr(Str::ButtonCancel), cmCancel, bfNormal);
    dlg->insert(add);
    dlg->insert(cancel);
    add->select(); // Enter adds right away; Shift+Tab goes to the server list

    ushort result = TProgram::application->execView(dlg);
    std::string chosen;
    if (result == cmOK) {
        size_t idx = (size_t)combo->value;
        if (idx < serverNames.size()) chosen = serverNames[idx];
    }
    TObject::destroy(dlg);
    return chosen;
}
