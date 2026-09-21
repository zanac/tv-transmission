#include "TorrentListWindow.h"
#include "TorrentDetailsWindow.h"
#include "TorrentFilesWindow.h"
#include "Strings.h"
#include "../TextUtil.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

#define Uses_TProgram
#define Uses_TDeskTop
#define Uses_TMenuItem
#define Uses_TMenu
#define Uses_TMenuPopup
#define Uses_TSubMenu
#define Uses_MsgBox
#define Uses_TKeys
#include <tvision/tv.h>
#include "App.h"

namespace {

// "servername — Torrents" — the server name leads, since with more
// than one of these windows potentially open at once (see App's own
// constructor), that's the part actually telling them apart; the
// translated "Torrents" after it is just there for anyone glancing at
// the title bar without already knowing what this app's windows are.
// `connectionLost` appends a translated "(offline)" marker — set
// whenever the most recent refresh attempt (sync or async — see
// TorrentListWindow::updateTitleForConnectionState()) failed, cleared
// the moment one succeeds again. A persistent marker in the title bar
// rather than a messageBox popping up every refresh cycle: a server
// that's actually down would otherwise mean a fresh popup every
// refreshIntervalSeconds until it's back, which is far more disruptive
// than something glanceable that's just... there until it isn't.
std::string buildWindowTitle(const std::string& serverName, bool connectionLost) {
    std::string title = serverName + " \xE2\x80\x94 " + tr(Str::WindowTitleTorrentList);
    if (connectionLost) title += " " + std::string(tr(Str::WindowTitleOffline));
    return title;
}

// Builds "[███████░░░░░░░░░]  42%" — the block characters (U+2588 full
// block / U+2591 light shade) are a near-universal convention for
// filled/empty progress in any UTF-8 terminal; the count of each is
// exactly proportional to percentDone, so the bar visibly fills up as
// the torrent approaches 100%.
//
// On Windows specifically, these two showed up as a row of "?" instead
// (reported directly, with a screenshot) — the Windows console isn't
// reliably in UTF-8 mode the way a Linux/macOS terminal already is, so
// a raw UTF-8 string handed to it (not going through tvision's own
// internal frame-drawing, which apparently already accounts for this —
// window borders drew correctly in the same screenshot) can't be
// counted on to render. '#'/'.' are plain 7-bit ASCII, so they're safe
// in any codepage regardless — a real visible fill/empty bar there
// beats a technically-nicer one that shows as "?????????".
#ifdef _WIN32
constexpr char kBarFilledChar = '#';
constexpr char kBarEmptyChar = '.';
#endif

constexpr int kBarInnerW = 16;
std::string buildProgressBar(double percentDone) {
    int filled = static_cast<int>(std::lround(percentDone * kBarInnerW));
    if (filled < 0) filled = 0;
    if (filled > kBarInnerW) filled = kBarInnerW;
    std::string bar = "[";
#ifdef _WIN32
    for (int i = 0; i < filled; i++) bar += kBarFilledChar;
    for (int i = 0; i < kBarInnerW - filled; i++) bar += kBarEmptyChar;
#else
    for (int i = 0; i < filled; i++) bar += "\u2588";
    for (int i = 0; i < kBarInnerW - filled; i++) bar += "\u2591";
#endif
    bar += "]";
    char pct[8];
    std::snprintf(pct, sizeof(pct), " %3.0f%%", percentDone * 100.0);
    bar += pct;
    return bar;
}

// Transmission's own sentinels for uploadRatio: -1 = not available (e.g.
// no data transferred yet), -2 = infinite (uploaded something with
// nothing downloaded, e.g. a torrent added as a seed). Shown as "—"/"∞"
// rather than the literal negative number, which would look like an
// error rather than a special case.
std::string formatRatio(double ratio) {
    if (ratio <= -2.0) return "\xE2\x88\x9E";    // ∞
    if (ratio < 0.0) return "\xE2\x80\x94";       // —
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%.2f", ratio);
    return buf;
}

// Transmission's own sentinels for eta: -1 = not available, -2 =
// unknown — both shown as "—" rather than a negative second count.
std::string formatEta(int64_t etaSeconds) {
    if (etaSeconds < 0) return "\xE2\x80\x94"; // —
    int64_t hours = etaSeconds / 3600;
    int64_t minutes = (etaSeconds % 3600) / 60;
    char buf[32];
    if (hours > 0) std::snprintf(buf, sizeof(buf), "%lldh %lldm", (long long)hours, (long long)minutes);
    else std::snprintf(buf, sizeof(buf), "%lldm", (long long)minutes);
    return buf;
}

std::string formatPriority(int priority) {
    if (priority < 0) return tr(Str::PriorityLow);
    if (priority > 0) return tr(Str::PriorityHigh);
    return tr(Str::PriorityNormal);
}

// tr_torrent_activity values (Transmission RPC): 0=stopped,
// 1=check-wait, 2=checking, 3=download-wait, 4=downloading,
// 5=seed-wait, 6=seeding.
//
// A row's color reflects the torrent's status at a glance; an error
// takes priority over the status-based color since it's the most
// actionable state to notice.
TColorAttr statusRowColor(const Torrent& t) {
    if (!t.errorString.empty()) return TColorAttr(0x1C); // error: light red on blue
    switch (t.status) {
        case 4: return TColorAttr(0x1B); // downloading: light cyan on blue
        case 6: return TColorAttr(0x1A); // seeding: light green on blue
        case 0: return TColorAttr(0x17); // stopped: light gray on blue (dimmer)
        case 1: case 2: case 3: case 5:
                return TColorAttr(0x1E); // checking/queued: yellow on blue
        default: return TColorAttr(0x1F); // fallback: white on blue
    }
}

// "Queued" here means 1/3/5: already started (waiting for its turn),
// as opposed to genuinely stopped (0).
bool isStopped(const Torrent& t) { return t.status == 0; }
bool isQueued(const Torrent& t) { return t.status == 1 || t.status == 3 || t.status == 5; }

void setCmd(TView* v, ushort cmd, bool enable) {
    if (enable) v->enableCommand(cmd);
    else v->disableCommand(cmd);
}

// `fmt` is one of our own tr() strings with a single "%s" placeholder;
// `value` is a plain argument to it, not itself interpreted as a format
// string, so a torrent name containing a literal '%' can't cause any
// issue here (unlike passing it directly to printf as the format).
std::string formatMessage(const char* fmt, const std::string& value) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), fmt, value.c_str());
    return buf;
}

// Case-insensitive substring search — an empty `needle` (no name filter
// set) always matches, same as the filter not existing.
bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
        [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
    return it != haystack.end();
}

// A torrent is shown only if it satisfies EVERY active filter (AND, not
// OR) — see TorrentFilter's own comment in AppSettings.h.
bool passesFilter(const Torrent& t, const TorrentFilter& f) {
    if (!containsCaseInsensitive(t.name, f.nameContains)) return false;
    switch (t.status) {
        case 0: return f.showStopped;
        case 1: return f.showCheckWait;
        case 2: return f.showChecking;
        case 3: return f.showDownloadWait;
        case 4: return f.showDownloading;
        case 5: return f.showSeedWait;
        case 6: return f.showSeeding;
        default: return true;
    }
}

} // namespace

TorrentListWindow::TorrentListWindow(const TRect& bounds, const std::string& serverName, TransmissionClient& client,
                                      SortColumn initialSort, bool initialAscending,
                                      TorrentFilter initialFilter,
                                      const std::vector<int>& initialColumnWidths,
                                      const std::vector<int>& initialColumnOrder,
                                      const std::vector<bool>& initialColumnVisible,
                                      SortChangedCallback onSortChanged,
                                      FilterChangedCallback onFilterChanged,
                                      const std::vector<int>& initialTrackerColumnWidths,
                                      const std::vector<int>& initialTrackerColumnOrder,
                                      const std::vector<bool>& initialTrackerColumnVisible,
                                      const std::vector<int>& initialPeerColumnWidths,
                                      const std::vector<int>& initialPeerColumnOrder,
                                      const std::vector<bool>& initialPeerColumnVisible)
    : TWindowInit(&TWindow::initFrame), // virtual base: must be initialized here,
                                         // by the most-derived class — TGridWindow's
                                         // own initialization of it doesn't propagate
                                         // through another level of inheritance
      // fullScreen=true: always exactly fills the desktop, tracking its
      // own size as the terminal itself is resized (TWindow's own
      // default growMode — gfGrowAll|gfGrowRel, set unconditionally in
      // its own constructor — already does that automatically; nothing
      // extra needed here for it). More than one of these can be open
      // at once now (one per configured server, stacked — see App's own
      // constructor and the "Connections" menu for how one is brought
      // to the front over the others), unlike the single always-
      // maximized window TGridWindow's own fullScreen mode was
      // originally built for — but the same flags=0 (no move/resize/
      // zoom/close) applies to each one independently either way, and
      // is exactly what's wanted here: never smaller than the whole
      // desktop, and only ever closed by the Connection dialog's own
      // "[-]" removing that server (see App::showConnectionDialog()),
      // never from the window itself.
      TGridWindow(bounds, buildWindowTitle(serverName, /*connectionLost=*/false), /*fullScreen=*/true,
                  gvResizableColumns | gvReorderableColumns | gvMultiSelect),
      client_(client),
      serverName_(serverName),
      initialTrackerColumnWidths_(initialTrackerColumnWidths),
      initialTrackerColumnOrder_(initialTrackerColumnOrder),
      initialTrackerColumnVisible_(initialTrackerColumnVisible),
      initialPeerColumnWidths_(initialPeerColumnWidths),
      initialPeerColumnOrder_(initialPeerColumnOrder),
      initialPeerColumnVisible_(initialPeerColumnVisible),
      filter_(std::move(initialFilter)),
      sortColumn_(initialSort), sortAscending_(initialAscending),
      onSortChanged_(std::move(onSortChanged)),
      onFilterChanged_(std::move(onFilterChanged)) {
    setupColumns(initialColumnWidths);
    applyColumnLabels();
    // Restores a persisted arrangement without needing any reaction from
    // this window: reordering only changes where columns are drawn, not
    // what data they show (see TGridView's logical/visual index split),
    // so unlike the sort indicator just below there's nothing else to
    // keep in sync here.
    grid()->setColumnOrder(initialColumnOrder);
    setColumnVisibility(initialColumnVisible);
    // Reflects the persisted sort in the header's "^"/"v" indicator
    // without going through setSortChangedCallback()'s callback: the
    // data isn't loaded yet (refresh() below sorts it, using
    // sortColumn_/sortAscending_ directly), so there's nothing to
    // re-sort in reaction to this — it would just be redundant.
    grid()->setSortIndicator((int)sortColumn_, sortAscending_);

    grid()->setCellTextCallback([this](int row, int col) -> std::string {
        if (row < 0 || row >= (int)visible_.size()) return "";
        const Torrent& t = visible_[row];
        switch (static_cast<SortColumn>(col)) {
            case SortColumn::Name:   return t.name;
            case SortColumn::Done:   return buildProgressBar(t.percentDone);
            case SortColumn::Size:   return formatSize(t.sizeBytes);
            case SortColumn::Down: {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "D:%7.1fKB/s", t.rateDownload / 1024.0);
                return buf;
            }
            case SortColumn::Up: {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "U:%7.1fKB/s", t.rateUpload / 1024.0);
                return buf;
            }
            case SortColumn::Added:  return formatUnixTimestamp(t.addedDate);
            case SortColumn::Status: return trTorrentStatus(t.status);
            case SortColumn::Ratio:      return formatRatio(t.uploadRatio);
            case SortColumn::Uploaded:   return formatSize(t.uploadedEver);
            case SortColumn::Downloaded: return formatSize(t.downloadedEver);
            case SortColumn::Location:   return t.downloadDir;
            case SortColumn::Eta:        return formatEta(t.eta);
            case SortColumn::Peers:      return std::to_string(t.peersConnected);
            case SortColumn::QueuePosition: return std::to_string(t.queuePosition + 1); // shown 1-based
            case SortColumn::Priority:   return formatPriority(t.bandwidthPriority);
            case SortColumn::CompletedDate:
                return t.doneDate > 0 ? formatUnixTimestamp(t.doneDate) : "\xE2\x80\x94"; // —
        }
        return "";
    });

    grid()->setRowColorCallback([this](int row, bool focused) -> TColorAttr {
        if (focused) return TColorAttr(0xF0); // current row: black on white, regardless of status
        if (row < 0 || row >= (int)visible_.size()) return TColorAttr(0x1F);
        return statusRowColor(visible_[row]);
    });

    // Only the name column is bold — see TGridView's own draw(): applied
    // regardless of focus, same as this window's rendering did before
    // the TGridView migration.
    grid()->setCellBoldCallback([](int, int col) { return col == (int)SortColumn::Name; });

    // Sorting-on-click itself (toggling direction, drawing "^"/"v") is
    // now handled entirely inside TGridView (see its own
    // setSortChangedCallback doc comment) — this callback only needs to
    // react to the choice: re-sort visible_ and let the caller persist
    // it, same as the old toggleSort() did.
    grid()->setSortChangedCallback([this](int col, bool ascending) {
        sortColumn_ = static_cast<SortColumn>(col);
        sortAscending_ = ascending;
        applyFilterAndSort();
        if (onSortChanged_) onSortChanged_(sortColumn_, sortAscending_);
    });
    grid()->setRowActivateCallback([this](int) { showDetailsForSelected(); });
    // Updates the Files panel's own content to whichever torrent is now
    // focused — fires on arrow-key navigation and a plain click alike
    // (see RowFocusFn's own comment in TGridView.h), which is what lets
    // it "follow" the selection the way the user described (not just
    // reacting to a double-click, which showDetailsForSelected() above
    // already handles separately). A no-op whenever the panel isn't
    // open — nothing to update if there's nothing showing it.
    //
    // Combined with updateCommandStates() into ONE callback — a SECOND
    // setRowFocusCallback() call used to follow this one, silently
    // replacing it (onRowFocus_ is a single std::function, an
    // assignment, not something both calls could accumulate into),
    // meaning updateCommandStates() ran on every row change but the
    // Files panel's own update here never did at all. Reported
    // directly ("clicking a torrent row doesn't update the file shown
    // in the Files panel") rather than caught by re-reading the code
    // first.
    grid()->setRowFocusCallback([this](int row) {
        if (filesPanel_ && row >= 0 && row < (int)visible_.size()) {
            filesPanel_->showTorrent(visible_[row].id, visible_[row].name);
        }
        updateCommandStates();
    });
    // Double-clicking the queue position column cycles through the four
    // queue-move actions instead of opening details, the same way
    // TorrentFilesWindow's own priority column cycles instead of
    // whatever a plain double-click would otherwise do — everything
    // else keeps opening details as before (returning false here leaves
    // the event alone, which is what lets RowActivateFn still fire for
    // every other column).
    grid()->setCellActivateCallback([this](int row, int col) -> bool {
        if (col == static_cast<int>(SortColumn::QueuePosition)) {
            cycleQueueActionForRow(row);
            return true;
        }
        if (col == static_cast<int>(SortColumn::Priority)) {
            cyclePriorityForRow(row);
            return true;
        }
        return false;
    });
    grid()->setRowContextCallback([this](int row, TPoint pos) { showContextMenuFor(row, pos); });
    grid()->setRowMiddleClickCallback([this](int) { showFilesForSelected(); });

    refresh();
}

void TorrentListWindow::setupColumns(const std::vector<int>& initialWidths) {
    grid()->clearColumns();

    TGridColumn name;
    name.header = tr(Str::HeaderName);
    name.width = 30;
    name.minWidth = 10;
    grid()->addColumn(name);

    TGridColumn done;
    done.header = tr(Str::HeaderDone);
    done.width = 23;       // "[" + 16 blocks + "]" + " NNN%" (5) = 23
    done.minWidth = 23;
    // Not resizable: buildProgressBar() always produces a fixed-width
    // 23-character string (a fixed 16-block-wide bar), so shrinking this
    // column would just truncate the bar/percentage rather than
    // reflowing anything — better to not offer a resize that can't do
    // anything useful.
    done.resizable = false;
    grid()->addColumn(done);

    TGridColumn size;
    size.header = tr(Str::HeaderSize);
    size.width = 10;
    size.minWidth = 6;
    size.align = TGridColumn::Align::Right;
    grid()->addColumn(size);

    TGridColumn down;
    down.header = tr(Str::HeaderDownload);
    down.width = 14;
    down.minWidth = 8;
    grid()->addColumn(down);

    TGridColumn up;
    up.header = tr(Str::HeaderUpload);
    up.width = 14;
    up.minWidth = 8;
    grid()->addColumn(up);

    TGridColumn added;
    added.header = tr(Str::HeaderAdded);
    added.width = 17;
    added.minWidth = 10;
    grid()->addColumn(added);

    TGridColumn status;
    status.header = tr(Str::HeaderStatus);
    status.width = 23;
    status.minWidth = 8;
    grid()->addColumn(status);

    // Hidden-by-default columns — useful to some, but not part of the
    // list's default look; shown via "Manage columns...".
    TGridColumn ratio;
    ratio.header = tr(Str::HeaderRatio);
    ratio.width = 8;
    ratio.minWidth = 5;
    ratio.align = TGridColumn::Align::Right;
    ratio.visible = false;
    grid()->addColumn(ratio);

    TGridColumn uploaded;
    uploaded.header = tr(Str::HeaderTotalUploaded);
    uploaded.width = 10;
    uploaded.minWidth = 6;
    uploaded.align = TGridColumn::Align::Right;
    uploaded.visible = false;
    grid()->addColumn(uploaded);

    TGridColumn downloaded;
    downloaded.header = tr(Str::HeaderTotalDownloaded);
    downloaded.width = 10;
    downloaded.minWidth = 6;
    downloaded.align = TGridColumn::Align::Right;
    downloaded.visible = false;
    grid()->addColumn(downloaded);

    TGridColumn location;
    location.header = tr(Str::HeaderLocation);
    location.width = 30;
    location.minWidth = 10;
    location.visible = false;
    grid()->addColumn(location);

    TGridColumn eta;
    eta.header = tr(Str::HeaderEta);
    eta.width = 10;
    eta.minWidth = 6;
    eta.align = TGridColumn::Align::Right;
    eta.visible = false;
    grid()->addColumn(eta);

    TGridColumn peers;
    peers.header = tr(Str::HeaderPeers);
    peers.width = 6;
    peers.minWidth = 4;
    peers.align = TGridColumn::Align::Right;
    peers.visible = false;
    grid()->addColumn(peers);

    TGridColumn queuePos;
    queuePos.header = tr(Str::HeaderQueuePosition);
    queuePos.width = 6;
    queuePos.minWidth = 4;
    queuePos.align = TGridColumn::Align::Right;
    queuePos.visible = false;
    grid()->addColumn(queuePos);

    TGridColumn priority;
    priority.header = tr(Str::HeaderPriority);
    priority.width = 8;
    priority.minWidth = 6;
    priority.visible = false;
    grid()->addColumn(priority);

    TGridColumn completedDate;
    completedDate.header = tr(Str::HeaderCompletedDate);
    completedDate.width = 17;
    completedDate.minWidth = 10;
    completedDate.visible = false;
    grid()->addColumn(completedDate);

    // Persisted widths from a previous session, applied on top of the
    // defaults above — only if there's exactly one per column: a
    // mismatched count (first run with no saved widths yet, or a
    // settings.json from before a column was added/removed) falls back
    // to the defaults just set rather than applying them partially or
    // out of order.
    if (initialWidths.size() == (size_t)grid()->columnCount()) {
        for (int i = 0; i < grid()->columnCount(); i++)
            grid()->setColumnWidth(i, initialWidths[i]);
    }
}

void TorrentListWindow::applyColumnLabels() {
    static const Str kLabels[] = {
        Str::HeaderName, Str::HeaderDone, Str::HeaderSize,
        Str::HeaderDownload, Str::HeaderUpload, Str::HeaderAdded, Str::HeaderStatus,
        Str::HeaderRatio, Str::HeaderTotalUploaded, Str::HeaderTotalDownloaded, Str::HeaderLocation,
        Str::HeaderEta, Str::HeaderPeers, Str::HeaderQueuePosition, Str::HeaderPriority,
        Str::HeaderCompletedDate,
    };
    int n = std::min(grid()->columnCount(), (int)(sizeof(kLabels) / sizeof(kLabels[0])));
    for (int i = 0; i < n; i++) grid()->column(i).header = tr(kLabels[i]);
}

void TorrentListWindow::retranslate() {
    // title is allocated with newStr() by TWindow's constructor (see
    // twindow.cpp) and freed with delete[] in its destructor — the same
    // pattern used for TStatusItem::text in BandwidthStatusLine.
    delete[] (char*)title;
    title = newStr(buildWindowTitle(serverName_, connectionLost_));
    applyColumnLabels(); // the sort "^"/"v" indicator is drawn by TGridView
                         // itself at draw time (see grid()->refresh() below),
                         // independent of the header label text — nothing
                         // to reapply here for it.
    grid()->refresh();
    drawView();
}

void TorrentListWindow::refresh() {
    allTorrents_ = client_.listTorrents();
    applyFilterAndSort();
    // lastError().empty() means the attempt that just ran succeeded —
    // see TransmissionClient::call()'s own comment on why every
    // attempt clears it first, not just failures setting it, which is
    // what makes this check meaningful right after the call above
    // rather than possibly stale from some earlier one.
    updateTitleForConnectionState(!client_.lastError().empty());
}

void TorrentListWindow::finishAsyncRefresh(CURLM* multi) {
    bool ok = false;
    std::vector<Torrent> result = client_.finishRefresh(multi, &ok);
    if (ok) {
        allTorrents_ = std::move(result);
        applyFilterAndSort();
    }
    // On failure, deliberately leaves allTorrents_ (and so the
    // displayed list) exactly as it was — a momentary network hiccup
    // shouldn't blank out the last known state, only mark the title
    // (see updateTitleForConnectionState() below) so it's visible
    // without being disruptive.
    updateTitleForConnectionState(!ok);
}

void TorrentListWindow::updateTitleForConnectionState(bool lost) {
    if (lost == connectionLost_) return; // no change — most ticks, most of the time
    connectionLost_ = lost;
    // Same alloc/free convention as retranslate() itself uses for this
    // same field — see its own comment.
    delete[] (char*)title;
    title = newStr(buildWindowTitle(serverName_, connectionLost_));
    drawView();
}

void TorrentListWindow::setFilter(TorrentFilter filter) {
    filter_ = std::move(filter);
    applyFilterAndSort();
}

void TorrentListWindow::setStatusPanelOpen(bool open, int width) {
    if (open == (statusPanel_ != nullptr)) {
        if (open) statusPanelWidth_ = width; // already open, just apply the new width
        relayoutPanels();
        return;
    }
    if (open) {
        statusPanelWidth_ = width;
        // Bounds given here don't matter beyond being non-degenerate —
        // relayoutPanels(), called at the end of this function, always
        // repositions/resizes it correctly before anything gets drawn.
        statusPanel_ = new StatusPanel(TRect(0, 0, statusPanelWidth_, 10), filter_,
            [this](const TorrentFilter& f) { if (onFilterChanged_) onFilterChanged_(f); });
        insert(statusPanel_);
    } else {
        // TView::destroy() (not `delete`) removes it from this
        // window's own subview list first — a bare `delete` here would
        // leave a dangling pointer in that list, the same
        // use-after-free class of bug already documented elsewhere in
        // this project (TWindow::close() on a still-modal dialog, see
        // "Fixed bugs" in README.md) for a different lifetime, same
        // underlying mistake.
        destroy(statusPanel_);
        statusPanel_ = nullptr;
    }
    relayoutPanels();
}

void TorrentListWindow::setFilesPanelOpen(bool open, int width) {
    if (open == (filesPanel_ != nullptr)) {
        if (open) filesPanelWidth_ = width;
        relayoutPanels();
        return;
    }
    if (open) {
        filesPanelWidth_ = width;
        // Bounds given here don't matter beyond being non-degenerate —
        // relayoutPanels(), called right after, always repositions/
        // resizes it correctly before anything gets drawn (same
        // convention as StatusPanel's own equivalent).
        //
        // A still-open scrollbar-rendering issue exists here (some of
        // grid_'s own embedded vertical TScrollBar's own track goes
        // unpainted for the visible-content rows — reported directly,
        // screenshot in hand). Tried three different fixes attempting
        // to address timing around this panel's own construction-time
        // size vs. its later resize to the real one (reordering
        // showTorrent() after relayoutPanels(), an explicit redraw()
        // after locate(), and constructing at the final size directly
        // instead of a placeholder-then-resize) — none of the first
        // two changed the symptom, and the third made it visibly
        // worse (the whole scrollbar losing its own color, not just
        // part of it), so reverted back to this simpler, known
        // baseline rather than keep compounding attempted fixes into
        // something harder to reason about. The actual root cause
        // wasn't found this pass — likely something inside TGridView's
        // own embedded TScrollBar not fully accounting for being
        // resized after its own construction, still needs isolating
        // directly (e.g. instrumenting TScrollBar's own drawPos()
        // itself) rather than guessed at from this file's own side.
        filesPanel_ = new FilesPanel(TRect(0, 0, filesPanelWidth_, 10), client_);
        insert(filesPanel_);
        relayoutPanels();
        int row = grid()->focusedRow();
        if (row >= 0 && row < (int)visible_.size()) {
            filesPanel_->showTorrent(visible_[row].id, visible_[row].name);
        }
        return;
    } else {
        destroy(filesPanel_);
        filesPanel_ = nullptr;
    }
    relayoutPanels();
}

void TorrentListWindow::relayoutPanels() {
    TRect r = getExtent();
    r.grow(-1, -1); // interior, same as TGridWindow's own constructor
    TRect gridRect = r;
    if (statusPanel_) {
        TRect panelRect = r;
        panelRect.b.x = r.a.x + statusPanelWidth_;
        statusPanel_->locate(panelRect);
        gridRect.a.x = panelRect.b.x + 1; // +1: a one-column gap is the
                                           // draggable boundary itself
                                           // (see handleEvent()) — not
                                           // owned by either view, so
                                           // it has to come from
                                           // somewhere between them
                                           // rather than either one's
                                           // own edge.
        statusPanelBorderX_ = panelRect.b.x;
    } else {
        statusPanelBorderX_ = -1;
    }
    if (filesPanel_) {
        TRect panelRect = r;
        panelRect.a.x = r.b.x - filesPanelWidth_;
        filesPanel_->locate(panelRect);
        // redraw(), not drawView() — drawView() only asks this panel's
        // own draw() to run once; redraw() (TGroup's own) explicitly
        // walks every nested child and asks EACH of them to redraw
        // itself unconditionally. grid_'s own embedded TScrollBar,
        // resized this same way when this panel's own size changes,
        // was found leaving part of its own track unpainted after a
        // resize (reported directly, screenshot in hand) — the same
        // "only ever repaints whatever's already invalidated, not its
        // own full extent" pattern already found and fixed once for
        // this project's own TPanelBackground, this time surfacing in
        // a plain stock tvision widget nested two levels deep instead.
        filesPanel_->redraw();
        gridRect.b.x = panelRect.a.x - 1; // same one-column-gap reasoning,
                                           // mirrored on this side
        filesPanelBorderX_ = panelRect.a.x - 1;
    } else {
        filesPanelBorderX_ = -1;
    }
    grid()->locate(gridRect);
    // Explicit — this window's own draw() (not any child's) is what
    // paints the collapse/expand arrows, including the ones on this
    // window's own frame when a panel is closed (column 0 or
    // size.x-1, outside any child's own bounds entirely, so no
    // child's own redraw would ever reach them). Nothing above this
    // point asks THIS window to redraw itself — only its children,
    // each individually — so without this, closing a panel correctly
    // relaid out the grid but left the reopen arrow simply never
    // drawn (found directly: read the actual cell at column 0 after
    // closing Status, and it was still just the plain frame
    // character, not the arrow drawn() was supposed to have painted
    // there).
    drawView();
}

void TorrentListWindow::changeBounds(const TRect& bounds) {
    TWindow::changeBounds(bounds);
    relayoutPanels();
}

void TorrentListWindow::handleEvent(TEvent& event) {
    // Ctrl+Left/Right: keyboard-driven resize, contextual on whichever
    // panel currently has keyboard focus (`current`, TGroup's own
    // direct-child focus tracking) — the same two entry points the
    // Panels menu's own "Resize ..." commands reach (App's own
    // resizeStatusPanelForFocused()/resizeFilesPanelForFocused(), which
    // call these same two methods), just reached directly here instead
    // of via a menu round-trip, and only when a panel itself (not the
    // main grid) is what's currently focused — Ctrl+Left/Right with the
    // grid focused does nothing here, on purpose: there's no panel
    // context to resize in that case.
    //
    // Checked and handled BEFORE calling TGridWindow::handleEvent()
    // below, not after — found directly (a temporary diagnostic print,
    // removed again once done, showed the event simply never arriving
    // here at all except for Enter) that whatever currently has focus
    // deeper inside the panel (e.g. the name filter's own TInputLine)
    // consumes Ctrl+Left/Right first for its own purposes (moving the
    // text cursor a word at a time, a standard TInputLine behavior) if
    // the base class's own handleEvent() — which dispatches down to
    // `current` and, from there, recursively into whatever CURRENT
    // holds — gets to run first.
    if (event.what == evKeyDown &&
        (event.keyDown.keyCode == kbCtrlLeft || event.keyDown.keyCode == kbCtrlRight)) {
        if (statusPanel_ && current == statusPanel_) {
            keyboardResizeStatusPanel();
            clearEvent(event);
            return;
        }
        if (filesPanel_ && current == filesPanel_) {
            keyboardResizeFilesPanel();
            clearEvent(event);
            return;
        }
    }
    // Collapse/expand arrow handles (draw()'s own comment shows exactly
    // where these get painted) — checked here, before
    // TGridWindow::handleEvent() below, for the same reason
    // Ctrl+Left/Right is checked here rather than after: a click on the
    // window's own frame (the reopen case, arrow on column 0 or
    // size.x-1) would otherwise reach TFrame's own click handling
    // first, which treats a frame click as the start of a move/resize
    // drag — never reaching this window's own check at all. A click
    // on the divider itself (the collapse case) isn't at risk the same
    // way, but is checked in the same place for one consistent spot
    // rather than splitting this feature across two.
    //
    // Posts the exact same command a Panels-menu click would (rather
    // than calling setStatusPanelOpen()/setFilesPanelOpen() directly)
    // via putEvent() — TView's own standard way to inject a command
    // into the pending queue, propagating up through the owner chain
    // (TView::putEvent(), tview.cpp) until TProgram's own queue picks
    // it up on the next getEvent() — so this reaches App's own
    // cmToggleStatusPanel/cmToggleFilesPanel handling exactly as a
    // real menu selection would: persisting the new open/closed state
    // right away and rebuilding the Panels menu itself, instead of
    // only changing this window's own layout and leaving both of
    // those for shutDown() to eventually catch up on.
    if (event.what == evMouseDown) {
        TPoint local = makeLocal(event.mouse.where);
        TRect r = getExtent();
        r.grow(-1, -1);
        int midY = r.a.y + (r.b.y - r.a.y) / 2;
        bool onArrowRow = (local.y == midY || local.y == midY + 1);
        if (onArrowRow) {
            TEvent cmdEvent;
            cmdEvent.what = evCommand;
            cmdEvent.message.infoPtr = nullptr;
            if (statusPanelBorderX_ >= 0 && local.x == statusPanelBorderX_) {
                cmdEvent.message.command = cmToggleStatusPanel;
                putEvent(cmdEvent);
                clearEvent(event);
                return;
            }
            if (statusPanelBorderX_ < 0 && local.x == 0) {
                cmdEvent.message.command = cmToggleStatusPanel;
                putEvent(cmdEvent);
                clearEvent(event);
                return;
            }
            if (filesPanelBorderX_ >= 0 && local.x == filesPanelBorderX_) {
                cmdEvent.message.command = cmToggleFilesPanel;
                putEvent(cmdEvent);
                clearEvent(event);
                return;
            }
            if (filesPanelBorderX_ < 0 && local.x == size.x - 1) {
                cmdEvent.message.command = cmToggleFilesPanel;
                putEvent(cmdEvent);
                clearEvent(event);
                return;
            }
        }
    }
    TGridWindow::handleEvent(event);
    if (event.what == evMouseDown && statusPanelBorderX_ >= 0) {
        TPoint local = makeLocal(event.mouse.where);
        if (local.x == statusPanelBorderX_) {
            if (event.mouse.eventFlags & meDoubleClick) {
                statusPanelWidth_ = kStatusPanelDefaultWidth;
                relayoutPanels();
            } else {
                dragResizeStatusPanel(event);
            }
            clearEvent(event);
            return;
        }
    }
    if (event.what == evMouseDown && filesPanelBorderX_ >= 0) {
        TPoint local = makeLocal(event.mouse.where);
        if (local.x == filesPanelBorderX_) {
            if (event.mouse.eventFlags & meDoubleClick) {
                filesPanelWidth_ = kFilesPanelDefaultWidth;
                relayoutPanels();
            } else {
                dragResizeFilesPanel(event);
            }
            clearEvent(event);
        }
    }
}

void TorrentListWindow::draw() {
    TGridWindow::draw();
    // Drawn AFTER the base class's own draw() — this is deliberately
    // on top of the grid/panel content it already painted, not a
    // background fill like StatusPanel/FilesPanel's own draw()
    // overrides. Full interior height, same rect relayoutPanels() uses
    // for everything else here, so the line always lines up with
    // whatever's actually on either side of it, panel width drag
    // included.
    TRect r = getExtent();
    r.grow(-1, -1);
    TDrawBuffer b;
    b.moveStr(0, "\xE2\x94\x82", TColorAttr(0x1F)); // │ (U+2502, UTF-8) —
        // moveStr, not moveChar: moveChar only takes a single raw
        // byte, which can't hold a multi-byte UTF-8 sequence (found
        // before this ever ran — moveChar's own signature takes `char`,
        // not a string). Standard app blue, matching everything else
        // here.
    if (statusPanelBorderX_ >= 0) {
        for (int y = r.a.y; y < r.b.y; y++) writeLine(statusPanelBorderX_, y, 1, 1, b);
    }
    if (filesPanelBorderX_ >= 0) {
        for (int y = r.a.y; y < r.b.y; y++) writeLine(filesPanelBorderX_, y, 1, 1, b);
    }

    // Collapse/expand handles — two rows at vertical center (not one:
    // asked for directly, wider and easier to hit than a single cell),
    // computed from the same interior rect everything else here uses,
    // so they stay centered regardless of terminal height. Drawn last,
    // on top of the divider line itself (or the window's own frame,
    // when the matching panel is closed) — see handleEvent()'s own
    // comment for how a click on one of these same four spots is
    // recognized and acted on.
    int midY = r.a.y + (r.b.y - r.a.y) / 2;
    TDrawBuffer arrow;
    if (statusPanelBorderX_ >= 0) {
        // Open: left-pointing arrow on the divider itself — closes it.
        // (Originally drawn right-pointing here — reported directly as
        // visually backwards from what it should mean, with the fix
        // described as simply swapping left/right throughout, which is
        // exactly what this and the other three below do.)
        arrow.moveStr(0, "\xE2\x97\x84", TColorAttr(0x1F)); // ◄
        writeLine(statusPanelBorderX_, midY, 1, 1, arrow);
        writeLine(statusPanelBorderX_, midY + 1, 1, 1, arrow);
    } else {
        // Closed: right-pointing arrow on the window's own left frame —
        // reopens it. Same color as the frame itself would otherwise
        // show there, so it reads as part of the frame rather than a
        // patch of mismatched color glued onto it.
        arrow.moveStr(0, "\xE2\x96\xBA", TColorAttr(0x71)); // ►
        writeLine(0, midY, 1, 1, arrow);
        writeLine(0, midY + 1, 1, 1, arrow);
    }
    if (filesPanelBorderX_ >= 0) {
        arrow.moveStr(0, "\xE2\x96\xBA", TColorAttr(0x1F)); // ►
        writeLine(filesPanelBorderX_, midY, 1, 1, arrow);
        writeLine(filesPanelBorderX_, midY + 1, 1, 1, arrow);
    } else {
        arrow.moveStr(0, "\xE2\x97\x84", TColorAttr(0x71)); // ◄
        writeLine(size.x - 1, midY, 1, 1, arrow);
        writeLine(size.x - 1, midY + 1, 1, 1, arrow);
    }
}

void TorrentListWindow::dragResizeStatusPanel(TEvent& event) {
    // Same live drag-and-relayout loop as TGridView's own column
    // dragResize() (TGridView.cpp) — tracks the mouse while the button
    // stays down, updating the width and relaying out on every move,
    // rather than only committing once on release.
    int startX = event.mouse.where.x;
    int startWidth = statusPanelWidth_;
    // kStatusPanelMaxWidth is a fixed ceiling; this window's own
    // current width is a separate, dynamic one — a narrow terminal
    // shouldn't let the panel grow wide enough to leave the grid with
    // no meaningful space at all, whatever kStatusPanelMaxWidth itself
    // says.
    int dynamicMax = std::min(kStatusPanelMaxWidth, size.x - 22);
    while (mouseEvent(event, evMouseMove)) {
        int delta = event.mouse.where.x - startX;
        int newWidth = startWidth + delta;
        if (newWidth < kStatusPanelMinWidth) newWidth = kStatusPanelMinWidth;
        if (newWidth > dynamicMax) newWidth = dynamicMax;
        statusPanelWidth_ = newWidth;
        relayoutPanels();
    }
}

void TorrentListWindow::dragResizeFilesPanel(TEvent& event) {
    // Mirror image of dragResizeStatusPanel() above — dragging left
    // GROWS this panel (it's pinned to the right edge), the opposite
    // sign from the status panel's own drag, otherwise identical.
    int startX = event.mouse.where.x;
    int startWidth = filesPanelWidth_;
    int dynamicMax = std::min(kFilesPanelMaxWidth, size.x - 22);
    while (mouseEvent(event, evMouseMove)) {
        int delta = startX - event.mouse.where.x;
        int newWidth = startWidth + delta;
        if (newWidth < kFilesPanelMinWidth) newWidth = kFilesPanelMinWidth;
        if (newWidth > dynamicMax) newWidth = dynamicMax;
        filesPanelWidth_ = newWidth;
        relayoutPanels();
    }
}

void TorrentListWindow::keyboardResizeStatusPanel() {
    if (!statusPanel_) return; // shouldn't happen — App's own caller
                                // already checked isStatusPanelOpen()
    int originalWidth = statusPanelWidth_;
    int dynamicMax = std::min(kStatusPanelMaxWidth, size.x - 22);
    TEvent event;
    for (;;) {
        // Same "pump events in a loop" primitive TGridView::
        // startKeyboardResize() itself uses (see its own comment there
        // for why getEvent() specifically, not mouseEvent() — this
        // isn't continuing a drag already in progress).
        getEvent(event);
        if (event.what != evKeyDown) continue;
        switch (event.keyDown.keyCode) {
            case kbLeft:
                if (statusPanelWidth_ > kStatusPanelMinWidth) {
                    statusPanelWidth_--;
                    relayoutPanels();
                }
                break;
            case kbRight:
                if (statusPanelWidth_ < dynamicMax) {
                    statusPanelWidth_++;
                    relayoutPanels();
                }
                break;
            case kbEnter:
                return; // confirmed at the current width
            case kbEsc:
                statusPanelWidth_ = originalWidth;
                relayoutPanels();
                return; // cancelled: reverted to the width it had on entry
            default:
                break; // any other key: ignored, keep waiting
        }
    }
}

void TorrentListWindow::keyboardResizeFilesPanel() {
    if (!filesPanel_) return;
    int originalWidth = filesPanelWidth_;
    int dynamicMax = std::min(kFilesPanelMaxWidth, size.x - 22);
    TEvent event;
    for (;;) {
        getEvent(event);
        if (event.what != evKeyDown) continue;
        switch (event.keyDown.keyCode) {
            // Mirrored from the status panel's own above — pinned to
            // the right edge, so Right (not Left) is the one that
            // SHRINKS it, matching dragResizeFilesPanel()'s own
            // reversed delta sign.
            case kbLeft:
                if (filesPanelWidth_ < dynamicMax) {
                    filesPanelWidth_++;
                    relayoutPanels();
                }
                break;
            case kbRight:
                if (filesPanelWidth_ > kFilesPanelMinWidth) {
                    filesPanelWidth_--;
                    relayoutPanels();
                }
                break;
            case kbEnter:
                return;
            case kbEsc:
                filesPanelWidth_ = originalWidth;
                relayoutPanels();
                return;
            default:
                break;
        }
    }
}

std::vector<int> TorrentListWindow::columnWidths() const {
    std::vector<int> widths;
    widths.reserve(grid()->columnCount());
    for (int i = 0; i < grid()->columnCount(); i++)
        widths.push_back(grid()->column(i).width);
    return widths;
}

std::vector<int> TorrentListWindow::columnOrder() const {
    return grid()->columnOrder();
}

std::vector<bool> TorrentListWindow::columnVisibility() const {
    std::vector<bool> vis(kTorrentColumnCount, true);
    for (int i = 0; i < kTorrentColumnCount; i++) vis[i] = grid()->isColumnVisible(i);
    return vis;
}

void TorrentListWindow::setColumnVisibility(const std::vector<bool>& visible) {
    for (int i = 0; i < kTorrentColumnCount; i++) {
        // A missing entry falls back to that column's own default —
        // true for the original 7, false for the 9 added later (see
        // setupColumns()) — rather than always true. That matters for
        // a settings.json saved before those 9 existed: it only has 7
        // entries, and without this, the missing 9 would silently come
        // back shown instead of hidden-by-default as intended.
        bool defaultShown = (i < 7);
        bool shown = (i < (int)visible.size()) ? visible[i] : defaultShown;
        grid()->setColumnVisible(i, shown);
    }
}

void TorrentListWindow::applyFilterAndSort() {
    visible_.clear();
    visible_.reserve(allTorrents_.size());
    for (const auto& t : allTorrents_)
        if (passesFilter(t, filter_)) visible_.push_back(t);

    // Always compare "ascending" but with the arguments swapped for
    // descending order, instead of negating the result: negating `less`
    // to get `greater` breaks the strict-weak-ordering std::sort
    // requires when two elements are equal (a<b false AND b<a false, but
    // !less(a,b) would still be "true" both ways).
    std::sort(visible_.begin(), visible_.end(),
        [this](const Torrent& a, const Torrent& b) {
            const Torrent& x = sortAscending_ ? a : b;
            const Torrent& y = sortAscending_ ? b : a;
            switch (sortColumn_) {
                case SortColumn::Name:   return x.name < y.name;
                case SortColumn::Done:   return x.percentDone < y.percentDone;
                case SortColumn::Size:   return x.sizeBytes < y.sizeBytes;
                case SortColumn::Down:   return x.rateDownload < y.rateDownload;
                case SortColumn::Up:     return x.rateUpload < y.rateUpload;
                case SortColumn::Added:  return x.addedDate < y.addedDate;
                case SortColumn::Status: return x.status < y.status;
                case SortColumn::Ratio:         return x.uploadRatio < y.uploadRatio;
                case SortColumn::Uploaded:      return x.uploadedEver < y.uploadedEver;
                case SortColumn::Downloaded:    return x.downloadedEver < y.downloadedEver;
                case SortColumn::Location:      return x.downloadDir < y.downloadDir;
                case SortColumn::Eta:           return x.eta < y.eta;
                case SortColumn::Peers:         return x.peersConnected < y.peersConnected;
                case SortColumn::QueuePosition: return x.queuePosition < y.queuePosition;
                case SortColumn::Priority:      return x.bandwidthPriority < y.bandwidthPriority;
                case SortColumn::CompletedDate: return x.doneDate < y.doneDate;
            }
            return false;
        });

    grid()->setRowCount((int)visible_.size());
    grid()->refresh();
    updateCommandStates();
}

const Torrent* TorrentListWindow::selectedTorrent() const {
    int row = grid()->focusedRow();
    if (row < 0 || row >= (int)visible_.size()) return nullptr;
    return &visible_[row];
}

std::vector<const Torrent*> TorrentListWindow::targetTorrents() const {
    std::vector<const Torrent*> out;
    if (grid()->isInSelectionMode()) {
        for (int row : grid()->selectedRows()) {
            if (row >= 0 && row < (int)visible_.size()) out.push_back(&visible_[row]);
        }
        if (!out.empty()) return out;
        // In selection mode but nothing checked yet — fall through to
        // the focused row below, same as not being in selection mode
        // at all, rather than an action silently doing nothing.
    }
    if (const Torrent* t = selectedTorrent()) out.push_back(t);
    return out;
}

void TorrentListWindow::updateCommandStates() {
    // enableCommand()/disableCommand() are process-wide, not per-window
    // (see setState()'s own override below, which is what re-syncs them
    // the moment THIS window becomes the active one) — so this only
    // means anything when called on whichever window is CURRENTLY
    // active. Without this guard, an unfocused window's own periodic
    // refresh() (every open window refreshes on the same shared timer —
    // see App::idle() — not just the focused one, and refresh() always
    // ends by calling this) would silently clobber the focused window's
    // own command state moments after a focus change, with whatever its
    // own selected torrent happens to need instead — exactly the
    // "briefly correct, then Start re-enables itself" symptom this was
    // written to fix.
    if ((state & sfActive) == 0) return;

    const Torrent* t = selectedTorrent();
    if (!t) {
        // Nothing selected (e.g. empty list): no per-torrent action
        // makes sense.
        setCmd(this, cmStartTorrent, false);
        setCmd(this, cmStopTorrent, false);
        setCmd(this, cmRemoveTorrent, false);
        setCmd(this, cmDeleteTorrentWithData, false);
        setCmd(this, cmVerifyTorrent, false);
        setCmd(this, cmReannounceTorrent, false);
        setCmd(this, cmStartNowTorrent, false);
        setCmd(this, cmShowDetails, false);
        setCmd(this, cmShowFiles, false);
        return;
    }
    bool stopped = isStopped(*t);
    bool queued = isQueued(*t);
    bool active = !stopped;

    setCmd(this, cmStartTorrent, stopped);           // already running/queued: nothing to start
    setCmd(this, cmStopTorrent, active);              // already stopped: nothing to stop
    setCmd(this, cmRemoveTorrent, true);              // always possible
    setCmd(this, cmDeleteTorrentWithData, true);       // always possible
    setCmd(this, cmVerifyTorrent, true);               // Transmission allows this in any state
    setCmd(this, cmReannounceTorrent, active);         // only meaningful while talking to trackers
    setCmd(this, cmStartNowTorrent, stopped || queued); // only useful if not already transferring
    setCmd(this, cmShowDetails, true);                 // always possible
    setCmd(this, cmShowFiles, true);                   // always possible
}

void TorrentListWindow::setState(ushort aState, Boolean enable) {
    TGridWindow::setState(aState, enable);
    // sfActive: this window just became (enable=True) or stopped being
    // (enable=False) the desktop's current one — only the "became"
    // direction needs anything here; the OTHER window gaining focus
    // will run its own updateCommandStates() right after via this same
    // override.
    if ((aState & sfActive) != 0 && enable) {
        updateCommandStates();
    }
}

void TorrentListWindow::showContextMenuFor(int /*row*/, TPoint screenPos) {
    // TMenuBox/TMenuPopup size themselves from their content and anchor
    // at bounds.a, expanding toward bounds.b — a small bounds.b here
    // (rather than a comfortably large one) would make it anchor
    // backwards from the click point instead of growing rightward/
    // downward from it (see getRect() in tvision's tmenubox.cpp).
    TRect r(screenPos.x, screenPos.y, screenPos.x + 40, screenPos.y + 10);
    // See App::initMenuBar()'s own comment on nesting a TSubMenu this
    // way — same reasoning applies here.
    TSubMenu* queueMenu = new TSubMenu(tr(Str::MenuQueue), kbNoKey);
    *queueMenu +
        *new TMenuItem(tr(Str::MenuQueueMoveTop), cmQueueMoveTop, kbNoKey) +
        *new TMenuItem(tr(Str::MenuQueueMoveUp), cmQueueMoveUp, kbNoKey) +
        *new TMenuItem(tr(Str::MenuQueueMoveDown), cmQueueMoveDown, kbNoKey) +
        *new TMenuItem(tr(Str::MenuQueueMoveBottom), cmQueueMoveBottom, kbNoKey);
    TSubMenu* priorityMenu = new TSubMenu(tr(Str::MenuPriority), kbNoKey);
    *priorityMenu +
        *new TMenuItem(tr(Str::MenuPriorityLow), cmSetPriorityLow, kbNoKey) +
        *new TMenuItem(tr(Str::MenuPriorityNormal), cmSetPriorityNormal, kbNoKey) +
        *new TMenuItem(tr(Str::MenuPriorityHigh), cmSetPriorityHigh, kbNoKey);
    // operator+(TMenuItem&, TMenuItem&) walks to the end of the first
    // item's existing chain and appends the second one there (see
    // menu.cpp), mutating that chain in place — so `items` (bound to
    // the very first item) already reflects anything appended to it
    // afterward below, without needing to be reassigned.
    TMenuItem& items =
        *new TMenuItem(tr(Str::MenuStart), cmStartTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuStartNow), cmStartNowTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuStop), cmStopTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuVerify), cmVerifyTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuReannounce), cmReannounceTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuRemove), cmRemoveTorrent, kbNoKey) +
        *new TMenuItem(tr(Str::MenuDeleteWithData), cmDeleteTorrentWithData, kbNoKey) +
        *new TMenuItem(tr(Str::MenuShowDetails), cmShowDetails, kbNoKey) +
        *new TMenuItem(tr(Str::MenuShowFiles), cmShowFiles, kbNoKey) +
        static_cast<TMenuItem&>(*queueMenu) +
        static_cast<TMenuItem&>(*priorityMenu);
    // Only meaningful — and only shown — while there's a selection to
    // cancel. Reuses cmSelectMultiple itself rather than a separate
    // command: its own handler (see App.cpp) already does exactly
    // "leave selection mode if already in it", which is exactly what
    // this needs since the item's only ever added when that's the case.
    if (grid()->isInSelectionMode()) {
        items + *new TMenuItem(tr(Str::MenuCancelSelection), cmSelectMultiple, kbNoKey);
    }
    TMenu* menu = new TMenu(items);
    auto* popup = new TMenuPopup(r, menu);
    // execView() (inherited from TProgram/TApplication) inserts the
    // popup, runs its own event loop until a choice is made or it's
    // dismissed, then removes it — the same mechanism tvision's own
    // pull-down submenus use internally (see newSubView()/execView() in
    // tmnuview.cpp) and the same one this app already uses for its own
    // modal dialogs (see App.cpp's execView(dlg) calls).
    ushort chosen = TProgram::application->execView(popup);
    TObject::destroy(popup);
    if (chosen != 0 && commandEnabled(chosen)) {
        // Re-emit exactly as a button or menu item would (see
        // TButton::press() in tbutton.cpp): this is the same command
        // value App::handleEvent already dispatches for the Torrent
        // menu and the status bar, so it reaches the same handling
        // without duplicating it here.
        TEvent e;
        e.what = evCommand;
        e.message.command = chosen;
        e.message.infoPtr = this;
        putEvent(e);
    }
}

void TorrentListWindow::showDetailsForSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    // Copied before the loop below: getTorrentDetails()/refresh() calls
    // partway through could otherwise invalidate the Torrent* pointers
    // targetTorrents() returned (they point into visible_).
    std::vector<int> ids;
    for (const Torrent* t : targets) ids.push_back(t->id);

    TDeskTop* deskTop = TProgram::deskTop;
    for (int id : ids) {
        // Look for an already-open details window for this same torrent
        // id before creating a new one — same deskTop->last/next
        // traversal already used for the "Window list" dialog (see
        // App.cpp). Order doesn't matter here either: we're searching
        // for a specific id, not relying on position.
        bool foundExisting = false;
        if (deskTop->last) {
            TView* p = deskTop->last;
            do {
                p = p->next;
                if (auto* existing = dynamic_cast<TorrentDetailsWindow*>(p)) {
                    if (existing->torrentId() == id) {
                        existing->select(); // bring the existing one to front instead
                        foundExisting = true;
                        break;
                    }
                }
            } while (p != deskTop->last);
        }
        if (foundExisting) continue;

        // The main list only carries listTorrents()'s lightweight fields
        // (see TransmissionClient.h) — the details window needs more
        // (location, privacy, magnet link, piece info, all-time transfer
        // totals, activity/elapsed-time fields), fetched here on demand
        // rather than on every periodic refresh.
        Torrent details = client_.getTorrentDetails(id);
        if (auto* win = createTorrentDetailsWindow(details, client_,
                                                    initialTrackerColumnWidths_,
                                                    initialTrackerColumnOrder_,
                                                    initialTrackerColumnVisible_,
                                                    initialPeerColumnWidths_,
                                                    initialPeerColumnOrder_,
                                                    initialPeerColumnVisible_))
            TProgram::application->insertWindow(win);
    }
    grid()->exitSelectionMode();
}

void TorrentListWindow::showFilesForSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    // Copied before the loop below for the same reason
    // showDetailsForSelected() does: names/ids point into visible_,
    // which a refresh() partway through the loop could invalidate.
    std::vector<std::pair<int, std::string>> idsAndNames;
    for (const Torrent* t : targets) idsAndNames.emplace_back(t->id, t->name);

    TDeskTop* deskTop = TProgram::deskTop;
    for (const auto& [id, name] : idsAndNames) {
        // Same "find an already-open one for this torrent id" check as
        // showDetailsForSelected() above, for the same reason: bring the
        // existing files window to front instead of opening a duplicate.
        bool foundExisting = false;
        if (deskTop->last) {
            TView* p = deskTop->last;
            do {
                p = p->next;
                if (auto* existing = dynamic_cast<TorrentFilesWindow*>(p)) {
                    if (existing->torrentId() == id) {
                        existing->select();
                        foundExisting = true;
                        break;
                    }
                }
            } while (p != deskTop->last);
        }
        if (foundExisting) continue;

        if (auto* win = createTorrentFilesWindow(id, name, client_))
            TProgram::application->insertWindow(win);
    }
    grid()->exitSelectionMode();
}

void TorrentListWindow::startSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.startTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::stopSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.stopTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::removeSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    std::string msg = (targets.size() == 1)
        ? formatMessage(tr(Str::ConfirmRemoveTorrent), targets[0]->name)
        : formatMessage(tr(Str::ConfirmRemoveTorrentsMulti), std::to_string(targets.size()));
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    for (const Torrent* t : targets) client_.removeTorrent(t->id, /*deleteLocalData=*/false);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::deleteWithDataSelected() {
    auto targets = targetTorrents();
    if (targets.empty()) return;
    std::string msg = (targets.size() == 1)
        ? formatMessage(tr(Str::ConfirmDeleteTorrentWithData), targets[0]->name)
        : formatMessage(tr(Str::ConfirmDeleteTorrentsWithDataMulti), std::to_string(targets.size()));
    if (messageBox(msg, mfConfirmation | mfYesButton | mfNoButton) != cmYes) return;
    for (const Torrent* t : targets) client_.removeTorrent(t->id, /*deleteLocalData=*/true);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::startNowSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.startTorrentNow(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::verifySelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.verifyTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::reannounceSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.reannounceTorrent(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::queueMoveTopForSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.queueMoveTop(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::queueMoveUpForSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.queueMoveUp(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::queueMoveDownForSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.queueMoveDown(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::queueMoveBottomForSelected() {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.queueMoveBottom(t->id);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::setPriorityForSelected(int priority) {
    auto targets = targetTorrents();
    for (const Torrent* t : targets) client_.setPriority(t->id, priority);
    grid()->exitSelectionMode();
    refresh();
}

void TorrentListWindow::cycleQueueActionForRow(int row) {
    if (row < 0 || row >= (int)visible_.size()) return;
    int id = visible_[row].id;
    switch (queueActionCycle_) {
        case 0: client_.queueMoveTop(id); break;
        case 1: client_.queueMoveUp(id); break;
        case 2: client_.queueMoveDown(id); break;
        case 3: client_.queueMoveBottom(id); break;
    }
    queueActionCycle_ = (queueActionCycle_ + 1) % 4;
    refresh();
}

void TorrentListWindow::cyclePriorityForRow(int row) {
    if (row < 0 || row >= (int)visible_.size()) return;
    const Torrent& t = visible_[row];
    int next = (t.bandwidthPriority <= -1) ? 0 : (t.bandwidthPriority == 0) ? 1 : -1;
    client_.setPriority(t.id, next);
    refresh();
}

double TorrentListWindow::totalDownloadRate() const {
    double total = 0.0;
    for (const auto& t : visible_) total += t.rateDownload;
    return total;
}

double TorrentListWindow::totalUploadRate() const {
    double total = 0.0;
    for (const auto& t : visible_) total += t.rateUpload;
    return total;
}
