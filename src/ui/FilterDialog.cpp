#include "FilterDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TSItem
#define Uses_TEvent
#include <tvision/tv.h>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

// Local to this dialog: scoped to its own handleEvent (same reasoning
// as the other dedicated commands in this app, e.g.
// TorrentDetailsWindow's cmApplySpeedLimits) — Reset clears the fields
// in place without closing the dialog, so it isn't cmOK/cmCancel.
constexpr ushort cmResetFilters = 250;

class FilterDialogImpl : public TDialog {
public:
    FilterDialogImpl(const TRect& r, TStringView title)
        : TWindowInit(&TDialog::initFrame), TDialog(r, title) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what == evCommand && event.message.command == cmResetFilters) {
            resetFields();
            clearEvent(event);
        }
    }

    TInputLine* nameField = nullptr;
    TCheckBoxes* statusBoxes = nullptr;

private:
    void resetFields() {
        std::vector<char> empty(129, 0);
        nameField->setData(empty.data());
        nameField->drawView();

        ushort allChecked = 0x7F; // 7 bits: every status shown
        statusBoxes->setData(&allChecked);
        statusBoxes->drawView();
    }
};

} // namespace

TDialog* createFilterDialog(const TorrentFilter& current, FilterDialogFields& fields) {
    TRect r(0, 0, 46, 18);
    auto* dlg = new FilterDialogImpl(r, tr(Str::DialogTitleFilters));
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 42, 3), tr(Str::LabelFilterName)));
    fields.nameContains = new TInputLine(TRect(2, 3, 42, 4), 128);
    std::vector<char> nameBuf(129, 0);
    std::snprintf(nameBuf.data(), nameBuf.size(), "%s", current.nameContains.c_str());
    fields.nameContains->setData(nameBuf.data());
    dlg->insert(fields.nameContains);

    dlg->insert(new TStaticText(TRect(2, 5, 42, 6), tr(Str::LabelFilterStatusSection)));

    // 7 items, tr_torrent_activity order (0..6) — same order
    // TorrentFilter's show* fields and the checked-bits below use.
    // Width: TCluster draws each item as "[ ] " (5 columns) + label (see
    // drawMultiBox() in tvision's tcluster.cpp) — 40 comfortably covers
    // the longest translated status string across all 5 languages
    // ("In attesa di verifica"/"In attesa di download", 22 chars) + 5.
    fields.statusCheckboxes = new TCheckBoxes(TRect(2, 6, 42, 13),
        new TSItem(tr(Str::TorrentStatusStopped),
        new TSItem(tr(Str::TorrentStatusCheckWait),
        new TSItem(tr(Str::TorrentStatusChecking),
        new TSItem(tr(Str::TorrentStatusDownloadWait),
        new TSItem(tr(Str::TorrentStatusDownloading),
        new TSItem(tr(Str::TorrentStatusSeedWait),
        new TSItem(tr(Str::TorrentStatusSeeding), nullptr))))))));
    ushort checked = (current.showStopped ? 0x01 : 0) |
                     (current.showCheckWait ? 0x02 : 0) |
                     (current.showChecking ? 0x04 : 0) |
                     (current.showDownloadWait ? 0x08 : 0) |
                     (current.showDownloading ? 0x10 : 0) |
                     (current.showSeedWait ? 0x20 : 0) |
                     (current.showSeeding ? 0x40 : 0);
    fields.statusCheckboxes->setData(&checked);
    dlg->insert(fields.statusCheckboxes);

    dlg->nameField = fields.nameContains;
    dlg->statusBoxes = fields.statusCheckboxes;

    dlg->insert(new TButton(TRect(2, 15, 14, 17), tr(Str::ButtonReset), cmResetFilters, bfNormal));
    dlg->insert(new TButton(TRect(18, 15, 28, 17), tr(Str::ButtonOK), cmOK, bfDefault));
    dlg->insert(new TButton(TRect(30, 15, 40, 17), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

TorrentFilter filterDialogResult(const FilterDialogFields& fields) {
    TorrentFilter result;
    if (fields.nameContains) {
        char buf[129] = {0};
        fields.nameContains->getData(buf);
        result.nameContains = buf;
    }
    if (fields.statusCheckboxes) {
        ushort checked = 0;
        fields.statusCheckboxes->getData(&checked);
        result.showStopped = (checked & 0x01) != 0;
        result.showCheckWait = (checked & 0x02) != 0;
        result.showChecking = (checked & 0x04) != 0;
        result.showDownloadWait = (checked & 0x08) != 0;
        result.showDownloading = (checked & 0x10) != 0;
        result.showSeedWait = (checked & 0x20) != 0;
        result.showSeeding = (checked & 0x40) != 0;
    }
    return result;
}
