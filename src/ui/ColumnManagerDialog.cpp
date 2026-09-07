#include "ColumnManagerDialog.h"
#include "TorrentListWindow.h"
#include "Strings.h"
#include "../tgridview/TGridView.h"

#define Uses_TButton
#define Uses_TEvent
#include <tvision/tv.h>
#include <vector>

namespace {

// Local to this dialog: scoped to its own handleEvent (same reasoning
// as the other dedicated commands in this app).
constexpr ushort cmColMgrResize = 250;
constexpr ushort cmColMgrMove   = 251;
constexpr ushort cmColMgrToggle = 252;
constexpr ushort cmColMgrReset  = 253;

// Same order as SortColumn — matches every other place in this app that
// maps a logical column index to its translated label (see
// TorrentListWindow::applyColumnLabels()).
const Str kColumnLabels[] = {
    Str::HeaderName, Str::HeaderDone, Str::HeaderSize,
    Str::HeaderDownload, Str::HeaderUpload, Str::HeaderAdded, Str::HeaderStatus,
};

class ColumnManagerDialogImpl : public TDialog {
public:
    ColumnManagerDialogImpl(const TRect& r, TStringView title, TorrentListWindow* target)
        : TWindowInit(&TDialog::initFrame), TDialog(r, title), target_(target) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what != evCommand) return;
        switch (event.message.command) {
            case cmColMgrResize: doResize(); clearEvent(event); break;
            case cmColMgrMove:   doMove();   clearEvent(event); break;
            case cmColMgrToggle: doToggle(); clearEvent(event); break;
            case cmColMgrReset:  doReset();  clearEvent(event); break;
        }
    }

    // Rebuilds the row->logical-column mapping from the target's
    // current visual order, then redraws — called once up front and
    // again after every action, since resizing doesn't change order but
    // moving and resetting both do.
    void refreshRows() {
        rowToLogicalCol = target_->columnOrder();
        if (metaGrid) metaGrid->refresh();
    }

    TGridView* metaGrid = nullptr;
    std::vector<int> rowToLogicalCol; // rowToLogicalCol[row] = logical SortColumn index

    // Small pass-throughs so the cellText callback (built outside this
    // class, in createColumnManagerDialog()) can read live values
    // without needing target_ itself to be public.
    std::vector<int> targetWidths() const { return target_->columnWidths(); }
    std::vector<bool> targetVisibility() const { return target_->columnVisibility(); }

private:
    // The one thing every action needs first: which real column the
    // currently-focused row of the meta-grid corresponds to, or -1 if
    // nothing sensible is focused (shouldn't normally happen with 7
    // fixed rows, but checked rather than assumed).
    int focusedLogicalCol() const {
        int row = metaGrid->focusedRow();
        if (row < 0 || row >= (int)rowToLogicalCol.size()) return -1;
        return rowToLogicalCol[row];
    }

    void doResize() {
        int col = focusedLogicalCol();
        if (col < 0) return;
        // Blocks until Enter/Esc — the exact same interactive loop the
        // header's own drag-to-resize and "Resize columns" used to
        // trigger (see TorrentListWindow::startColumnResize()).
        target_->startColumnResize(col);
        refreshRows();
    }

    void doMove() {
        int col = focusedLogicalCol();
        if (col < 0) return;
        target_->startColumnReorder(col); // blocks until Enter/Esc
        refreshRows();
        // The moved column's ROW in this meta-grid may have changed
        // (rows are shown in visual order — see refreshRows()) — follow
        // it, so repeated Move presses act on the same real column
        // instead of whatever now happens to sit in the old row.
        for (int i = 0; i < (int)rowToLogicalCol.size(); i++) {
            if (rowToLogicalCol[i] == col) { metaGrid->focusRow(i); break; }
        }
    }

    void doToggle() {
        int col = focusedLogicalCol();
        if (col < 0) return;
        auto vis = target_->columnVisibility();
        if (col < (int)vis.size()) vis[col] = !vis[col];
        target_->setColumnVisibility(vis);
        refreshRows();
    }

    void doReset() {
        target_->resetColumnLayout();
        refreshRows();
    }

    TorrentListWindow* target_;
};

} // namespace

TDialog* createColumnManagerDialog(TorrentListWindow* target) {
    TRect r(0, 0, 50, 18);
    auto* dlg = new ColumnManagerDialogImpl(r, tr(Str::DialogTitleColumnManager), target);
    dlg->options |= ofCentered;

    // The meta-grid itself is neither resizable nor reorderable (that
    // would be a strange thing to offer for a list OF columns) — see
    // gvNone. Each of its own 3 columns is likewise locked down
    // (sortable/resizable/movable all false): reordering "by which
    // property" or resizing this dialog's own layout isn't something
    // this dialog needs to support.
    TRect gridRect(2, 2, 48, 11);
    dlg->metaGrid = new TGridView(gridRect, gvNone);
    dlg->insert(dlg->metaGrid);

    TGridColumn nameCol;
    nameCol.header = tr(Str::LabelColumnManagerColumn);
    nameCol.width = 22;
    nameCol.sortable = false;
    nameCol.resizable = false;
    nameCol.movable = false;
    dlg->metaGrid->addColumn(nameCol);

    TGridColumn widthCol;
    widthCol.header = tr(Str::LabelColumnManagerWidth);
    widthCol.width = 8;
    widthCol.align = TGridColumn::Align::Right;
    widthCol.sortable = false;
    widthCol.resizable = false;
    widthCol.movable = false;
    dlg->metaGrid->addColumn(widthCol);

    TGridColumn visibleCol;
    visibleCol.header = tr(Str::LabelColumnManagerVisible);
    visibleCol.width = 10;
    visibleCol.sortable = false;
    visibleCol.resizable = false;
    visibleCol.movable = false;
    dlg->metaGrid->addColumn(visibleCol);

    dlg->metaGrid->setRowCount(7);
    dlg->metaGrid->setCellTextCallback([dlg](int row, int col) -> std::string {
        if (row < 0 || row >= (int)dlg->rowToLogicalCol.size()) return "";
        int logicalCol = dlg->rowToLogicalCol[row];
        switch (col) {
            case 0: return (logicalCol >= 0 && logicalCol < 7) ? tr(kColumnLabels[logicalCol]) : "";
            case 1: {
                auto widths = dlg->targetWidths();
                return (logicalCol < (int)widths.size()) ? std::to_string(widths[logicalCol]) : "";
            }
            case 2: {
                auto vis = dlg->targetVisibility();
                bool shown = logicalCol < (int)vis.size() ? vis[logicalCol] : true;
                return shown ? "[X]" : "[ ]";
            }
        }
        return "";
    });
    dlg->refreshRows();

    dlg->insert(new TButton(TRect(2, 12, 14, 14), tr(Str::ButtonResizeColumn), cmColMgrResize, bfNormal));
    dlg->insert(new TButton(TRect(15, 12, 25, 14), tr(Str::ButtonMoveColumn), cmColMgrMove, bfNormal));
    dlg->insert(new TButton(TRect(26, 12, 42, 14), tr(Str::ButtonToggleVisible), cmColMgrToggle, bfNormal));
    dlg->insert(new TButton(TRect(2, 15, 14, 17), tr(Str::ButtonReset), cmColMgrReset, bfNormal));
    dlg->insert(new TButton(TRect(37, 15, 47, 17), tr(Str::ButtonClose), cmCancel, bfDefault));

    dlg->selectNext(False);
    return dlg;
}
