#include "TrackerPeerWindow.h"
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

TrackerPeerWindow::TrackerPeerWindow(const TRect& bounds, TStringView title,
                                      int torrentId, TransmissionClient& client,
                                      const std::vector<int>& initialColumnWidths,
                                      const std::vector<int>& initialColumnOrder,
                                      const std::vector<bool>& initialColumnVisible,
                                      const std::vector<int>& initialPeerColumnWidths,
                                      const std::vector<int>& initialPeerColumnOrder,
                                      const std::vector<bool>& initialPeerColumnVisible)
    : TWindowInit(&TDialog::initFrame),
      TDialog(bounds, title),
      torrentId_(torrentId), client_(client),
      trackerColumnWidths_(initialColumnWidths),
      trackerColumnOrder_(initialColumnOrder),
      trackerColumnVisible_(initialColumnVisible),
      peerColumnWidths_(initialPeerColumnWidths),
      peerColumnOrder_(initialPeerColumnOrder),
      peerColumnVisible_(initialPeerColumnVisible) {
    options |= ofCentered;

    TRect r = getExtent();
    r.grow(-1, -1);
    r.b.y -= 3;   // room for the button row at the bottom
    int tabY = r.a.y;
    r.a.y += 1;   // room for the tab row at the top — the radio cluster below is itself only 1 row tall

    // A two-item TRadioButtons cluster rather than two ordinary
    // buttons standing in for a tab control tvision doesn't have
    // natively — its own marker glyph already shows which one is
    // selected, with keyboard navigation (arrow keys move AND select
    // immediately — see TTrackerPeerRadio's own doc comment) and focus
    // handling already built in, none of which needed reimplementing or
    // patching around the way two plain TButtons pretending to be tabs
    // did (see "Fixed bugs" in the README). height=1 in the bounds
    // below is what makes TCluster's own layout arrange the two items
    // side by side in one row instead of stacked vertically — its
    // column-vs-row layout switches on exactly that (see TCluster::
    // column() in tvision's own tcluster.cpp).
    TSItem* tabItems = new TSItem(tr(Str::TabTrackers), new TSItem(tr(Str::TabPeers), nullptr));
    // Spans the FULL row width (r.b.x, not just enough for the two
    // labels) so its own background — TCluster::drawMultiBox() already
    // fills its entire own bounds with one color before drawing
    // anything on top (see tcluster.cpp) — extends all the way to the
    // grid's own right edge instead of stopping right after "Peers",
    // matching the same row the grid's own header sits on below it.
    tabRadio_ = new TTrackerPeerRadio(TRect(r.a.x, tabY, r.b.x, tabY + 1), tabItems);
    tabRadio_->onChanged = [this](int item) {
        switchToTab(item == 0 ? Tab::Trackers : Tab::Peers);
    };
    insert(tabRadio_);

    // Resizable and reorderable; sortability is decided per tab (see
    // this class's own doc comment in the header for why neither one
    // currently allows it) — gvResizableColumns|gvReorderableColumns
    // applies to both regardless, since it's the grid's own state, not
    // rebuilt on every switchToTab() call the way the columns
    // themselves are.
    grid_ = new TGridView(r, gvResizableColumns | gvReorderableColumns);
    insert(grid_);

    grid_->setCellTextCallback([this](int row, int col) -> std::string {
        if (activeTab_ == Tab::Trackers) {
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
        } else {
            if (row < 0 || row >= (int)peers_.size()) return "";
            const Peer& p = peers_[row];
            char buf[32];
            switch (col) {
                case 0: return p.address + ":" + std::to_string(p.port);
                case 1: return p.clientName;
                case 2:
                    std::snprintf(buf, sizeof(buf), "%.0f%%", p.progress * 100.0);
                    return buf;
                case 3: return formatSize(p.rateToClient) + "/s";
                case 4: return formatSize(p.rateToPeer) + "/s";
                case 5: return p.flagStr;
            }
            return "";
        }
    });
    grid_->setRowActivateCallback([this](int) {
        // Only the Trackers tab has a details window of its own — a
        // double-click on a Peers row is simply a no-op, not a
        // dead-end error or a details window with nothing meaningful
        // to show (Peer has no equivalent of TrackerStat's own extra
        // fields that wouldn't fit in a table row to begin with).
        if (activeTab_ == Tab::Trackers) showDetailForSelected();
    });
    // No setRowColorCallback() here anymore — TGridView's own default
    // (white-on-blue/black-on-white-when-focused) already matches what
    // this used to set explicitly, since that became the generic
    // widget's own fallback instead of something every caller needed to
    // repeat.
    //
    // No setHeaderColorCallback() here either anymore — TGridView's own
    // default (yellow-on-blue, matching the main torrent list's own
    // header) already applies without asking for it, the same
    // unification already applied to row colors above.

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

    // Trackers shown first regardless of which tab was last active in
    // any other tracker window — see the constructor's own doc comment
    // in the header for why that's not itself persisted the way the
    // columns are.
    switchToTab(Tab::Trackers);
}

void TrackerPeerWindow::switchToTab(Tab tab) {
    // Captures whatever's currently in grid_ into the OUTGOING tab's
    // own remembered layout before rebuilding it out from under
    // itself — without this, switching to the other tab and back would
    // silently lose any resize/reorder done in THIS session that was
    // never explicitly saved via "Manage columns..." (see this
    // member's own comment in the header). A no-op the very first time
    // this runs (grid_ has no columns yet at all).
    if (grid_->columnCount() > 0) {
        std::vector<int> widths(grid_->columnCount());
        std::vector<bool> vis(grid_->columnCount());
        for (int i = 0; i < grid_->columnCount(); i++) {
            widths[i] = grid_->column(i).width;
            vis[i] = grid_->isColumnVisible(i);
        }
        std::vector<int> order = grid_->columnOrder();
        if (activeTab_ == Tab::Trackers) {
            trackerColumnWidths_ = widths; trackerColumnOrder_ = order; trackerColumnVisible_ = vis;
        } else {
            peerColumnWidths_ = widths; peerColumnOrder_ = order; peerColumnVisible_ = vis;
        }
    }

    activeTab_ = tab;

    grid_->clearColumns();
    if (tab == Tab::Trackers) {
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
    } else {
        TGridColumn addressCol;
        addressCol.header = tr(Str::HeaderPeerAddress);
        addressCol.width = 22;
        addressCol.minWidth = 10;
        addressCol.sortable = false;
        grid_->addColumn(addressCol);

        TGridColumn clientCol;
        clientCol.header = tr(Str::HeaderPeerClient);
        clientCol.width = 24;
        clientCol.minWidth = 8;
        clientCol.sortable = false;
        grid_->addColumn(clientCol);

        TGridColumn progressCol;
        progressCol.header = tr(Str::HeaderPeerProgress);
        progressCol.width = 8;
        progressCol.minWidth = 5;
        progressCol.align = TGridColumn::Align::Right;
        progressCol.sortable = false;
        grid_->addColumn(progressCol);

        TGridColumn downCol;
        downCol.header = tr(Str::HeaderPeerDown);
        downCol.width = 10;
        downCol.minWidth = 6;
        downCol.align = TGridColumn::Align::Right;
        downCol.sortable = false;
        grid_->addColumn(downCol);

        TGridColumn upCol;
        upCol.header = tr(Str::HeaderPeerUp);
        upCol.width = 10;
        upCol.minWidth = 6;
        upCol.align = TGridColumn::Align::Right;
        upCol.sortable = false;
        grid_->addColumn(upCol);

        TGridColumn flagsCol;
        flagsCol.header = tr(Str::HeaderPeerFlags);
        flagsCol.width = 8;
        flagsCol.minWidth = 5;
        flagsCol.sortable = false;
        grid_->addColumn(flagsCol);
    }

    // Applies whichever layout belongs to the tab just switched to —
    // same "exact count or fall back to the defaults just set" rule as
    // the constructor always used, now shared between both tabs and
    // both the very first switchToTab() call (initial* from the
    // constructor) and every later one (whatever was captured above).
    const std::vector<int>& widths = (tab == Tab::Trackers) ? trackerColumnWidths_ : peerColumnWidths_;
    const std::vector<int>& order = (tab == Tab::Trackers) ? trackerColumnOrder_ : peerColumnOrder_;
    const std::vector<bool>& vis = (tab == Tab::Trackers) ? trackerColumnVisible_ : peerColumnVisible_;
    if (widths.size() == (size_t)grid_->columnCount()) {
        for (int i = 0; i < grid_->columnCount(); i++)
            grid_->setColumnWidth(i, widths[i]);
    }
    grid_->setColumnOrder(order); // no-op if not a valid permutation — see TGridView::setColumnOrder()
    for (int i = 0; i < grid_->columnCount(); i++) {
        bool shown = (i < (int)vis.size()) ? vis[i] : true;
        grid_->setColumnVisible(i, shown);
    }

    refresh();
}

std::vector<int> TrackerPeerWindow::columnWidths() const {
    std::vector<int> widths(grid_->columnCount());
    for (int i = 0; i < grid_->columnCount(); i++) widths[i] = grid_->column(i).width;
    return widths;
}

std::vector<int> TrackerPeerWindow::columnOrder() const {
    return grid_->columnOrder();
}

std::vector<bool> TrackerPeerWindow::columnVisibility() const {
    std::vector<bool> vis(grid_->columnCount());
    for (int i = 0; i < grid_->columnCount(); i++) vis[i] = grid_->isColumnVisible(i);
    return vis;
}

void TrackerPeerWindow::refresh() {
    if (activeTab_ == Tab::Trackers) {
        trackers_ = client_.getTrackerStats(torrentId_);
        grid_->setRowCount((int)trackers_.size());
    } else {
        peers_ = client_.getPeers(torrentId_);
        grid_->setRowCount((int)peers_.size());
    }
    grid_->refresh();
}

void TrackerPeerWindow::showDetailForSelected() {
    int row = grid_->focusedRow();
    if (row < 0 || row >= (int)trackers_.size()) return;
    if (auto* win = createTrackerDetailWindow(trackers_[row]))
        TProgram::application->insertWindow(win);
}

void TrackerPeerWindow::handleEvent(TEvent& event) {
    TDialog::handleEvent(event);
    if (event.what != evCommand) return;
    switch (event.message.command) {
        case cmRefreshTrackers:      refresh();                    clearEvent(event); break;
        case cmCloseTrackers:        close();                       clearEvent(event); break;
    }
}

TDialog* createTrackerPeerWindow(int torrentId, const std::string& torrentName,
                                  TransmissionClient& client,
                                  const std::vector<int>& initialColumnWidths,
                                  const std::vector<int>& initialColumnOrder,
                                  const std::vector<bool>& initialColumnVisible,
                                  const std::vector<int>& initialPeerColumnWidths,
                                  const std::vector<int>& initialPeerColumnOrder,
                                  const std::vector<bool>& initialPeerColumnVisible) {
    TRect r(0, 0, 76, 23);
    std::string shortName = truncateUtf8(torrentName, 30);
    char titleBuf[128];
    std::snprintf(titleBuf, sizeof(titleBuf), tr(Str::WindowTitleTrackerList), shortName.c_str());
    return new TrackerPeerWindow(r, titleBuf, torrentId, client,
                                  initialColumnWidths, initialColumnOrder, initialColumnVisible,
                                  initialPeerColumnWidths, initialPeerColumnOrder, initialPeerColumnVisible);
}
