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
class TComboBox;

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
    // Rebuilds serverCombo_'s own item list from settings_.servers
    // (alphabetical, same as the constructor's own initial build) —
    // called after showConnectionDialog() adds, edits, or removes a
    // server, so the combo never shows a name that's been removed, or
    // misses one just added, without needing a restart. Keeps
    // whichever name is currently shown focused if it still exists in
    // the rebuilt list (falls back to the first entry otherwise, same
    // as buildServerComboItems() always does for a name it can't
    // find) — a no-op if serverCombo_ hasn't been created yet (it
    // always has been by the time this could actually be called, but
    // this stays a plain pointer check rather than an assumption, the
    // same caution TGridView applies to its own optional callbacks).
    void refreshServerCombo();
    // Keeps serverCombo_'s own shown/focused entry matching whichever
    // server's window actually has focus right now — called from
    // idle() (see its own comment: same "every tick, act on whichever
    // window currently has focus" pattern updateBandwidthStatus() and
    // the cmManageColumns enable/disable already follow there), so
    // switching windows any OTHER way than picking from the combo
    // itself (Ctrl+F6 "Next", Alt+0's window list, ...) still leaves it
    // showing the right name afterward. A no-op whenever there's
    // nothing to change: no focused torrent-list window at all (some
    // other kind of window has focus, or the combo itself does — left
    // alone rather than fighting the user's own click into it), or the
    // combo already shows the right name — that second check matters
    // because TComboBox::focusItem() (see TComboBox.cpp) broadcasts
    // unconditionally even when the index given is the one already
    // focused, so calling it every single idle tick without first
    // checking would mean a redundant broadcast (and, via handleEvent()
    // 's own cmComboBoxSelectionChanged case, a redundant re-select of
    // the already-focused window) on every tick rather than only when
    // something actually changed.
    void syncServerCombo();
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
    // The "pick a server, bring its window to the front" combo box
    // shown in its own reserved row directly below the menu bar — see
    // App::App() for why that row exists and how it's carved out of
    // deskTop's own extent, and refreshServerCombo() for how this stays
    // in sync with settings_.servers after the Connection dialog adds/
    // edits/removes one. Owned by the TGroup it's inserted into (this
    // app itself), same lifetime convention as every other view here —
    // not deleted explicitly.
    TComboBox* serverCombo_ = nullptr;
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
