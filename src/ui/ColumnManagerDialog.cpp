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
    Str::HeaderRatio, Str::HeaderTotalUploaded, Str::HeaderTotalDownloaded, Str::HeaderLocation,
    Str::HeaderEta, Str::HeaderPeers, Str::HeaderQueuePosition, Str::HeaderPriority,
    Str::HeaderCompletedDate,
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

    // Public (not private, like the other do*() actions below) so
    // double-clicking a row — wired via setRowActivateCallback() in
    // createColumnManagerDialog(), outside this class — can trigger the
    // exact same toggle the "Toggle visible" button does, rather than
    // duplicating its logic.
    void doToggle() {
        int col = focusedLogicalCol();
        if (col < 0) return;
        auto vis = target_->columnVisibility();
        if (col < (int)vis.size()) vis[col] = !vis[col];
        target_->setColumnVisibility(vis);
        refreshRows();
    }

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

    void doReset() {
        target_->resetColumnLayout();
        refreshRows();
    }

    TorrentListWindow* target_;
};

} // namespace

TDialog* createColumnManagerDialog(TorrentListWindow* target) {
    // Tall enough for the header row plus all kTorrentColumnCount rows
    // without needing to scroll, now that the list has more than just
    // the original 7 columns.
    TRect r(0, 0, 50, 27);
    auto* dlg = new ColumnManagerDialogImpl(r, tr(Str::DialogTitleColumnManager), target);
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

    dlg->metaGrid->setRowCount(kTorrentColumnCount);
    dlg->metaGrid->setCellTextCallback([dlg](int row, int col) -> std::string {
        if (row < 0 || row >= (int)dlg->rowToLogicalCol.size()) return "";
        int logicalCol = dlg->rowToLogicalCol[row];
        switch (col) {
            case 0:
                return (logicalCol >= 0 && logicalCol < kTorrentColumnCount)
                    ? tr(kColumnLabels[logicalCol]) : "";
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
    // Double-click a row: same action as the "Toggle visible" button —
    // TListViewer's own double-click/Enter broadcast (see TGridView's
    // setRowActivateCallback() doc comment), so there's no need to
    // reach for the button for the single most common thing to do here.
    dlg->metaGrid->setRowActivateCallback([dlg](int) { dlg->doToggle(); });
    // Without this, the row/no-row distinction relies entirely on
    // TListViewer's own default palette colors (see TGridRowsView::
    // draw()'s fallback in TGridView.cpp) — which, inside a TDialog,
    // turned out not to contrast enough to actually notice which row
    // was focused. Same fixed black-on-white-when-focused look already
    // used for the main torrent list, for the same reason: guaranteed
    // visible contrast regardless of whatever palette a dialog resolves
    // colors 1/2 to.
    dlg->metaGrid->setRowColorCallback([](int, bool focused) -> TColorAttr {
        return focused ? TColorAttr(0xF0) : TColorAttr(0x1F);
    });
    dlg->refreshRows();

    dlg->insert(new TButton(TRect(2, 21, 14, 23), tr(Str::ButtonResizeColumn), cmColMgrResize, bfNormal));
    dlg->insert(new TButton(TRect(15, 21, 25, 23), tr(Str::ButtonMoveColumn), cmColMgrMove, bfNormal));
    dlg->insert(new TButton(TRect(26, 21, 42, 23), tr(Str::ButtonToggleVisible), cmColMgrToggle, bfNormal));
    dlg->insert(new TButton(TRect(2, 24, 14, 26), tr(Str::ButtonReset), cmColMgrReset, bfNormal));
    dlg->insert(new TButton(TRect(37, 24, 47, 26), tr(Str::ButtonClose), cmCancel, bfDefault));

    dlg->selectNext(False);
    return dlg;
}
