#include "TGridColumnManagerDialog.h"

#define Uses_TButton
#define Uses_TEvent
#include <tvision/tv.h>
#include <vector>

namespace {

// Local to this dialog: scoped to its own handleEvent (same reasoning
// as every other dedicated command in this project).
constexpr ushort cmColMgrResize = 250;
constexpr ushort cmColMgrMove   = 251;
constexpr ushort cmColMgrToggle = 252;
constexpr ushort cmColMgrReset  = 253;

class TGridColumnManagerDialogImpl : public TDialog {
public:
    TGridColumnManagerDialogImpl(const TRect& r, const TGridColumnManagerLabels& labels,
                                  TGridView* target)
        : TWindowInit(&TDialog::initFrame), TDialog(r, labels.title.c_str()),
          labels_(labels), target_(target) {}

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
    std::vector<int> rowToLogicalCol; // rowToLogicalCol[row] = logical column index on target_

    TGridView* target() const { return target_; }
    const TGridColumnManagerLabels& labels() const { return labels_; }

    // Public (not private, like the other do*() actions below) so
    // double-clicking a row — wired via setRowActivateCallback() in
    // createColumnManagerDialog(), outside this class — can trigger the
    // exact same toggle the "Toggle visible" button does, rather than
    // duplicating its logic.
    void doToggle() {
        int col = focusedLogicalCol();
        if (col < 0) return;
        target_->setColumnVisible(col, !target_->isColumnVisible(col));
        refreshRows();
    }

private:
    int focusedLogicalCol() const {
        int row = metaGrid->focusedRow();
        if (row < 0 || row >= (int)rowToLogicalCol.size()) return -1;
        return rowToLogicalCol[row];
    }

    void doResize() {
        int col = focusedLogicalCol();
        if (col < 0) return;
        target_->startKeyboardResize(col); // blocks until Enter/Esc
        refreshRows();
    }

    void doMove() {
        int col = focusedLogicalCol();
        if (col < 0) return;
        target_->startKeyboardReorder(col); // blocks until Enter/Esc
        refreshRows();
        // The moved column's ROW in this meta-grid may have changed
        // (rows are shown in visual order — see refreshRows()) — follow
        // it, so repeated Move presses act on the same real column
        // instead of whatever now happens to sit in the old row.
        for (int i = 0; i < (int)rowToLogicalCol.size(); i++) {
            if (rowToLogicalCol[i] == col) { metaGrid->focusRow(i); break; }
        }
    }

    void doReset() {
        target_->resetColumns();
        refreshRows();
    }

    TGridColumnManagerLabels labels_;
    TGridView* target_;
};

} // namespace

TDialog* createColumnManagerDialog(TGridView* target, const TGridColumnManagerLabels& labels) {
    TRect r(0, 0, 50, 27);
    auto* dlg = new TGridColumnManagerDialogImpl(r, labels, target);
    dlg->options |= ofCentered;

    // The meta-grid itself is neither resizable nor reorderable (that
    // would be a strange thing to offer for a list OF columns) — see
    // gvNone. Each of its own 3 columns is likewise locked down
    // (sortable/resizable/movable all false): reordering "by which
    // property" or resizing this dialog's own layout isn't something
    // this dialog needs to support.
    TRect gridRect(2, 2, 48, 20);
    dlg->metaGrid = new TGridView(gridRect, gvNone);
    dlg->insert(dlg->metaGrid);

    TGridColumn nameCol;
    nameCol.header = labels.columnHeader;
    nameCol.width = 22;
    nameCol.sortable = false;
    nameCol.resizable = false;
    nameCol.movable = false;
    dlg->metaGrid->addColumn(nameCol);

    TGridColumn widthCol;
    widthCol.header = labels.widthHeader;
    widthCol.width = 8;
    widthCol.align = TGridColumn::Align::Right;
    widthCol.sortable = false;
    widthCol.resizable = false;
    widthCol.movable = false;
    dlg->metaGrid->addColumn(widthCol);

    TGridColumn visibleCol;
    visibleCol.header = labels.visibleHeader;
    visibleCol.width = 10;
    visibleCol.sortable = false;
    visibleCol.resizable = false;
    visibleCol.movable = false;
    dlg->metaGrid->addColumn(visibleCol);

    dlg->metaGrid->setRowCount(target->columnCount());
    dlg->metaGrid->setCellTextCallback([dlg](int row, int col) -> std::string {
        if (row < 0 || row >= (int)dlg->rowToLogicalCol.size()) return "";
        int logicalCol = dlg->rowToLogicalCol[row];
        TGridView* target = dlg->target();
        switch (col) {
            case 0: return target->column(logicalCol).header;
            case 1: return std::to_string(target->column(logicalCol).width);
            case 2: return target->isColumnVisible(logicalCol) ? dlg->labels().yes : dlg->labels().no;
        }
        return "";
    });
    // Without this, the focused-row/other-rows distinction relies
    // entirely on TListViewer's own default palette colors (see
    // TGridRowsView::draw()'s fallback in TGridView.cpp) — which,
    // inside a TDialog, don't contrast enough to actually notice which
    // row is focused. Fixed black-on-white-when-focused instead, so
    // this is guaranteed visible regardless of whatever palette the
    // host dialog resolves colors 1/2 to.
    dlg->metaGrid->setRowColorCallback([](int, bool focused) -> TColorAttr {
        return focused ? TColorAttr(0xF0) : TColorAttr(0x1F);
    });
    // Double-click a row: same action as the "Toggle visible" button —
    // TListViewer's own double-click/Enter broadcast (see TGridView's
    // setRowActivateCallback() doc comment), so there's no need to
    // reach for the button for the single most common thing to do here.
    dlg->metaGrid->setRowActivateCallback([dlg](int) { dlg->doToggle(); });
    dlg->refreshRows();

    dlg->insert(new TButton(TRect(2, 21, 14, 23), labels.resizeButton.c_str(), cmColMgrResize, bfNormal));
    dlg->insert(new TButton(TRect(15, 21, 25, 23), labels.moveButton.c_str(), cmColMgrMove, bfNormal));
    dlg->insert(new TButton(TRect(26, 21, 42, 23), labels.toggleVisibleButton.c_str(), cmColMgrToggle, bfNormal));
    dlg->insert(new TButton(TRect(2, 24, 14, 26), labels.resetButton.c_str(), cmColMgrReset, bfNormal));
    dlg->insert(new TButton(TRect(37, 24, 47, 26), labels.closeButton.c_str(), cmCancel, bfDefault));

    dlg->selectNext(False);
    return dlg;
}
