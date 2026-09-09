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

class App : public TApplication {
public:
    // `initialSettings` must be loaded (loadSettings()) and its language
    // applied (setLanguage()) BEFORE constructing App: initMenuBar()/
    // initStatusLine() are static and get invoked while TApplication's
    // base classes are being constructed, i.e. before any code in this
    // constructor's body can run. See main.cpp.
    explicit App(const AppSettings& initialSettings);

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
    TorrentListWindow* openServerWindow(const std::string& name, const TRect& bounds);
    void showAddTorrentDialog(const std::string& initialValue = "");
    void showConnectionDialog();
    void showServerSettingsDialog();
    void showFilterDialog();
    void showColumnManagerDialog();
    void showWindowListDialog();
    void showAboutDialog();
    void updateBandwidthStatus(); // updates the D:/U: text in the status bar, from the FOCUSED window's own server

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
