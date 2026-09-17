#pragma once

#define Uses_TApplication
#define Uses_TMenuBar
#define Uses_TStatusLine
#include <tvision/tv.h>

#include <chrono>
#include <map>
#include <memory>
#include <vector>
#include "../AppSettings.h"
#include "../rpc/TransmissionClient.h"

class TorrentListWindow;
class TGridView;
class StatusPanel;

class App : public TApplication {
public:
    // `initialSettings` must be loaded (loadSettings()) and its language
    // applied (setLanguage()) BEFORE constructing App: initMenuBar()/
    // initStatusLine() are static and get invoked while TApplication's
    // base classes are being constructed, i.e. before any code in this
    // constructor's body can run. See main.cpp.
    explicit App(const AppSettings& initialSettings);
    // Declared explicitly (not left to the implicit default) only to
    // clean up multiHandle_ itself — see the destructor's own comment
    // in App.cpp for why nothing else needs doing here, and for the
    // crash an earlier, more "defensive" version of this actually
    // caused by assuming otherwise.
    ~App() override;

    static TMenuBar* initMenuBar(TRect r);
    static TStatusLine* initStatusLine(TRect r);

    void handleEvent(TEvent& event) override;
    void idle() override; // checks whether it's time to refresh again
    void shutDown() override; // saves every open window's own layout, and the shared column widths, before exiting

private:
    // Opens (or, if one's already open for `name`, just focuses) a
    // torrent-list window for that configured server — used both by the
    // constructor (once per entry in settings_.servers, per this app's
    // own "every configured server opens on startup" behavior) and by
    // showConnectionDialog() (a server just added/edited there that
    // isn't already showing gets its own window the same way, rather
    // than needing a restart to see it).
    TorrentListWindow* openServerWindow(const std::string& name);
    void showAddTorrentDialog(const std::string& initialValue = "", const std::string& initialDestination = "");
    void showConnectionDialog();
    void showServerSettingsDialog();
    void showSessionStatsDialog();
    void showFilterDialog();
    void showColumnManagerDialog();
    void showWindowListDialog();
    void showAboutDialog();
    void updateBandwidthStatus(); // updates the D:/U: text in the status bar, from the FOCUSED window's own server
    // Rebuilds the "Connections" menu's own item list from
    // settings_.servers, marking whichever one's window currently has
    // focus (a bullet and the item's whole text in the menu's own
    // highlight color — see its own comment for why that's the closest
    // a text-mode menu gets to "bold" without custom drawing) — or a
    // single disabled "Empty" item if there are none configured at all.
    // Called both when the server LIST itself changes (added via Save,
    // removed via "[-]" — see showConnectionDialog()) and, from idle(),
    // whenever which one has FOCUS changes, however that happened
    // (clicking a different window directly, Window → Next, the Window
    // List dialog, or this very menu).
    void rebuildConnectionsMenu();
    void rebuildPanelsMenu();
    void toggleStatusPanel();
    void setStatusPanelVisible(bool visible);
    void layoutTorrentWindowsForPanel();
    void applyStatusPanelBits(ushort checked);

    // The TGridView belonging to whichever window currently has focus
    // (any TGridView-based window — a torrent list, the tracker list, a
    // torrent's files window, ...), or nullptr if the focused window has
    // none. "Manage columns..." is a single menu entry that acts on
    // this, rather than a separate entry per window — see App::idle()
    // (enables/disables the command as focus changes) and
    // showColumnManagerDialog() (acts on whatever this returns at the
    // moment the command fires).
    TGridView* focusedGrid() const;

    // Whichever torrent-list window currently has focus, or nullptr if
    // it's some other kind of window (or none at all) — every Torrent-
    // menu command (Start, Queue, Select Multiple, ...) acts on this
    // rather than on a single fixed window, now that more than one can
    // be open at once. Looked up fresh every time rather than cached:
    // these windows are independently closable (see TGridWindow's own
    // fullScreen=false path) and TWindow::close() destroys the window
    // outright, so a pointer held across event-loop turns could easily
    // end up dangling.
    TorrentListWindow* focusedListWindow() const;
    // Every currently open torrent-list window, in whatever order
    // TGroup's own child chain holds them — used where an action needs
    // ALL of them rather than just the focused one (idle()'s periodic
    // refresh, shutDown()'s layout save), for the same "don't cache
    // pointers across turns" reason as focusedListWindow() above.
    std::vector<TorrentListWindow*> allListWindows() const;
    // Closes every OTHER kind of window (Details, Files, Tracker — not
    // torrent-list windows themselves, which the caller closes
    // separately) still pointing at `client`'s own TransmissionClient
    // — see each of their own clientPtr() comments for why this is
    // needed before a server's client is actually destroyed (see
    // showConnectionDialog()): any of them left open past that point
    // would hold a dangling reference to a client that no longer
    // exists.
    void closeWindowsForClient(TransmissionClient* client) const;

    AppSettings settings_;
    // One TransmissionClient per configured server, keyed by the same
    // logical name as settings_.servers — each open TorrentListWindow
    // holds a reference to its own entry here (see TorrentListWindow's
    // own client_ member) rather than every window sharing one, since
    // they can each be talking to a different Transmission daemon at
    // once now. A std::map (not unordered_map, not a vector) so a
    // TorrentListWindow's own reference stays valid no matter what else
    // is inserted afterward — unlike a vector, inserting into a map
    // never invalidates existing elements' addresses.
    std::map<std::string, std::unique_ptr<TransmissionClient>> clients_;
    std::chrono::steady_clock::time_point lastRefresh_;
    // Shared by every window's own async refresh (see TorrentListWindow::
    // startAsyncRefresh()) rather than one CURLM per client — driving
    // one multi handle's own curl_multi_perform()/curl_multi_info_read()
    // once per idle() tick (see idle() itself) naturally covers however
    // many refreshes happen to be in flight at once, without needing to
    // loop over every client's own separate multi handle to ask each
    // one individually.
    CURLM* multiHandle_ = nullptr;
    // Whichever server's own window rebuildConnectionsMenu() last saw
    // focused, checked on every idle() tick — a plain string compare
    // against focusedListWindow()'s own current serverName() is enough
    // to tell whether focus actually moved since the last tick, so the
    // menu is only rebuilt when it needs to be, not on every single
    // tick regardless.
    std::string lastConnectionsFocusedServer_;
    StatusPanel* statusPanel_ = nullptr;
};

// Custom application commands (> tvision's cmUserBase)
const ushort cmAddTorrent       = 100;
const ushort cmStartTorrent     = 101;
const ushort cmStopTorrent      = 102;
const ushort cmRemoveTorrent    = 103;
const ushort cmSettings         = 104; // opens the Connection dialog — see App::showConnectionDialog()
const ushort cmBandwidthDisplay = 105; // non-clickable item in the status bar
const ushort cmShowWindowList   = 106;
const ushort cmVerifyTorrent    = 107;
const ushort cmReannounceTorrent= 108;
const ushort cmStartNowTorrent  = 109;
const ushort cmShowDetails      = 110;
const ushort cmDeleteTorrentWithData = 111;
const ushort cmAbout            = 112;
const ushort cmFilters          = 113;
const ushort cmManageColumns    = 114;
const ushort cmShowFiles        = 115;
const ushort cmSelectMultiple   = 116;
const ushort cmQueueMoveTop     = 117;
const ushort cmQueueMoveUp      = 118;
const ushort cmQueueMoveDown    = 119;
const ushort cmQueueMoveBottom  = 120;
const ushort cmServerSettings   = 121;
const ushort cmSetPriorityLow    = 122;
const ushort cmSetPriorityNormal = 123;
const ushort cmSetPriorityHigh   = 124;
const ushort cmSessionStats     = 125;
const ushort cmToggleStatusPanel = 126;
// Base for the "Connections" menu's own dynamic per-server commands
// (see App::rebuildConnectionsMenu()) — one entry per configured
// server, however many there are, so this needs real headroom rather
// than the next single free value the way every other command above
// gets one. 150 leaves a comfortable gap above cmServerSettings for
// any future *fixed* command to still fit without bumping into this
// range.
const ushort cmConnectionBase   = 150;
