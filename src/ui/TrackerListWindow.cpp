#include "TrackerListWindow.h"
#include "TrackerDetailWindow.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TButton
#define Uses_TProgram
#define Uses_TEvent
#include <tvision/tv.h>
#include <cstdio>

namespace {

// Transmission uses -1 for "not known yet" (e.g. before the first
// successful announce) — show "N/A" rather than a misleading number.
std::string formatCount(int v) {
    if (v < 0) return tr(Str::ValueNotAvailable);
    return std::to_string(v);
}

// Local to this window: scoped to its own handleEvent (same reasoning
// as TorrentDetailsWindow's cmApplySpeedLimits/cmCloseDetails).
constexpr ushort cmRefreshTrackers = 210;
constexpr ushort cmCloseTrackers = 211;

} // namespace

TrackerListWindow::TrackerListWindow(const TRect& bounds, TStringView title,
                                      int torrentId, TransmissionClient& client,
                                      const std::vector<int>& initialColumnWidths,
                                      const std::vector<int>& initialColumnOrder,
                                      const std::vector<bool>& initialColumnVisible)
    : TWindowInit(&TDialog::initFrame),
      TDialog(bounds, title),
      torrentId_(torrentId), client_(client) {
    options |= ofCentered;

    TRect r = getExtent();
    r.grow(-1, -1);
    r.b.y -= 3; // room for the button row at the bottom

    // Resizable and reorderable, but NOT sortable — see this class's own
    // doc comment in the header for why: Transmission already returns
    // trackers in tier order, which is the order that matters here, so
    // click-to-sort would work against that rather than help. That's a
    // row-order concern though, independent of the columns' own
    // width/position/visibility, which is what these two options (and
    // the "Columns..." button below) offer.
    grid_ = new TGridView(r, gvResizableColumns | gvReorderableColumns);
    insert(grid_);

    TGridColumn hostCol;
    hostCol.header = tr(Str::HeaderTrackerHost);
    hostCol.width = 30;
    hostCol.minWidth = 10;
    hostCol.sortable = false;
    grid_->addColumn(hostCol);

    TGridColumn tierCol;
    tierCol.header = tr(Str::HeaderTier);
    tierCol.width = 8;
    tierCol.minWidth = 6;
    tierCol.align = TGridColumn::Align::Right;
    tierCol.sortable = false;
    grid_->addColumn(tierCol);

    TGridColumn seedersCol;
    seedersCol.header = tr(Str::HeaderSeeders);
    seedersCol.width = 8;
    seedersCol.minWidth = 5;
    seedersCol.align = TGridColumn::Align::Right;
    seedersCol.sortable = false;
    grid_->addColumn(seedersCol);

    TGridColumn leechersCol;
    leechersCol.header = tr(Str::HeaderLeechers);
    leechersCol.width = 8;
    leechersCol.minWidth = 5;
    leechersCol.align = TGridColumn::Align::Right;
    leechersCol.sortable = false;
    grid_->addColumn(leechersCol);

    TGridColumn downloadedCol;
    downloadedCol.header = tr(Str::HeaderDownloaded);
    downloadedCol.width = 10;
    downloadedCol.minWidth = 6;
    downloadedCol.align = TGridColumn::Align::Right;
    downloadedCol.sortable = false;
    grid_->addColumn(downloadedCol);

    TGridColumn statusCol;
    statusCol.header = tr(Str::HeaderTrackerStatus);
    statusCol.width = 8;
    statusCol.minWidth = 6;
    statusCol.sortable = false;
    grid_->addColumn(statusCol);

    // Persisted widths from a previous session (or another already-open
    // tracker window), applied on top of the defaults above — only if
    // there's exactly one per column: a mismatched count (first run, or
    // a settings.json from before this window had persisted columns at
    // all) falls back to the defaults just set rather than applying
    // them partially or out of order. Same reasoning as
    // TorrentListWindow::setupColumns() for the main list.
    if (initialColumnWidths.size() == (size_t)grid_->columnCount()) {
        for (int i = 0; i < grid_->columnCount(); i++)
            grid_->setColumnWidth(i, initialColumnWidths[i]);
    }
    grid_->setColumnOrder(initialColumnOrder); // no-op if not a valid permutation — see TGridView::setColumnOrder()
    for (int i = 0; i < grid_->columnCount(); i++) {
        bool shown = (i < (int)initialColumnVisible.size()) ? initialColumnVisible[i] : true;
        grid_->setColumnVisible(i, shown);
    }

    grid_->setCellTextCallback([this](int row, int col) -> std::string {
        if (row < 0 || row >= (int)trackers_.size()) return "";
        const TrackerStat& t = trackers_[row];
        switch (col) {
            case 0: return t.host;
            case 1: return "Tier " + std::to_string(t.tier + 1);
            case 2: return formatCount(t.seederCount);
            case 3: return formatCount(t.leecherCount);
            case 4: return formatCount(t.downloadCount);
            case 5:
                return !t.hasAnnounced ? ""
                    : (t.lastAnnounceSucceeded ? tr(Str::TrackerStatusOk) : tr(Str::TrackerStatusError));
        }
        return "";
    });
    grid_->setRowActivateCallback([this](int) { showDetailForSelected(); });

    // "Columns..." is no longer a button here — it's now the single,
    // focus-aware "Manage columns..." menu entry (see App::
    // focusedGrid()/showColumnManagerDialog()), which acts on this
    // window's own grid_ automatically whenever this window has focus,
    // the same way it does for the main torrent list or a files window.
    int buttonY = r.b.y + 1;
    int x = r.a.x;
    insert(new TButton(TRect(x, buttonY, x + 12, buttonY + 2), tr(Str::ButtonRefresh), cmRefreshTrackers, bfDefault));
    x += 13;
    insert(new TButton(TRect(x, buttonY, x + 12, buttonY + 2), tr(Str::ButtonClose), cmCloseTrackers, bfNormal));

    refresh();
}

std::vector<int> TrackerListWindow::columnWidths() const {
    std::vector<int> widths(grid_->columnCount());
    for (int i = 0; i < grid_->columnCount(); i++) widths[i] = grid_->column(i).width;
    return widths;
}

std::vector<int> TrackerListWindow::columnOrder() const {
    return grid_->columnOrder();
}

std::vector<bool> TrackerListWindow::columnVisibility() const {
    std::vector<bool> vis(grid_->columnCount());
    for (int i = 0; i < grid_->columnCount(); i++) vis[i] = grid_->isColumnVisible(i);
    return vis;
}

void TrackerListWindow::refresh() {
    trackers_ = client_.getTrackerStats(torrentId_);
    grid_->setRowCount((int)trackers_.size());
    grid_->refresh();
}

void TrackerListWindow::showDetailForSelected() {
    int row = grid_->focusedRow();
    if (row < 0 || row >= (int)trackers_.size()) return;
    if (auto* win = createTrackerDetailWindow(trackers_[row]))
        TProgram::application->insertWindow(win);
}

void TrackerListWindow::handleEvent(TEvent& event) {
    TDialog::handleEvent(event);
    if (event.what != evCommand) return;
    switch (event.message.command) {
        case cmRefreshTrackers:     refresh();           clearEvent(event); break;
        case cmCloseTrackers:       close();              clearEvent(event); break;
    }
}

TDialog* createTrackerListWindow(int torrentId, const std::string& torrentName,
                                  TransmissionClient& client,
                                  const std::vector<int>& initialColumnWidths,
                                  const std::vector<int>& initialColumnOrder,
                                  const std::vector<bool>& initialColumnVisible) {
    TRect r(0, 0, 76, 23);
    std::string shortName = truncateUtf8(torrentName, 30);
    char titleBuf[128];
    std::snprintf(titleBuf, sizeof(titleBuf), tr(Str::WindowTitleTrackerList), shortName.c_str());
    return new TrackerListWindow(r, titleBuf, torrentId, client,
                                  initialColumnWidths, initialColumnOrder, initialColumnVisible);
}
