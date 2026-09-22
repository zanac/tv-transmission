#include "App.h"
#include "TorrentListWindow.h"
#include "TorrentDetailsWindow.h"
#include "TorrentFilesWindow.h"
#include "TrackerPeerWindow.h"
#include "AddTorrentDialog.h"
#include "../tvision-ext/TFolderBrowserDialog.h"
#include "ConnectionDialog.h"
#include "ServerSettingsDialog.h"
#include "SessionStatsDialog.h"
#include "../tvision-ext/TGridColumnManagerDialog.h"
#include "WindowListDialog.h"
#include "AboutDialog.h"
#include "BandwidthStatusLine.h"
#include "Strings.h"
#include "../Config.h"

#define Uses_TDeskTop
#define Uses_TGroup
#define Uses_TWindow
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TMenu
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TKeys
#define Uses_TEvent
#define Uses_TFileDialog
#define Uses_MsgBox
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iterator>
#include <vector>

namespace {
// TProgInit requires initMenuBar() to be static, so there's no `this`
// yet, at the point it runs, to hand this pointer to directly — set
// there (see its own comment), read back by App's own constructor and
// by rebuildConnectionsMenu() itself afterward. Only ever the
// "Connections" TSubMenu's own underlying TMenu (its `items`/`deflt`
// chain is what actually gets rebuilt) — every other menu in this app
// is static once built, so nothing else needs this. Safe as a single
// mutable global specifically because exactly one App instance ever
// exists in this process.
TMenu* g_connectionsMenu = nullptr;
// Same reasoning as g_connectionsMenu above: initMenuBar() is static
// (TProgInit's own requirement), so this is how the later
// rebuildPanelsMenu() call finds the same "Panels" submenu again.
TMenu* g_panelsMenu = nullptr;
}

App::App(const AppSettings& initialSettings)
    : TProgInit(&App::initStatusLine, &App::initMenuBar, &TApplication::initDeskTop),
      settings_(initialSettings) {
    // The global language was already set by main() BEFORE constructing
    // this object (see the comment in App.h): initMenuBar()/
    // initStatusLine() have therefore already read it correctly. No need
    // to redo it here.
    lastRefresh_ = std::chrono::steady_clock::now();
    // Shared by every window's own async refresh — see its own comment
    // in App.h. curl_global_init() itself doesn't need calling here:
    // libcurl's own docs say it's safe to skip in a single-threaded
    // program (this one) as long as at least one curl_easy_init() call
    // happens before any concurrent use, which TransmissionClient's own
    // constructor already guarantees for its own persistent handle.
    multiHandle_ = curl_multi_init();

    // Every configured server gets its own window on startup — see
    // AppSettings::servers' own comment on why there's no longer just
    // one "active" connection. Each one always exactly fills the
    // desktop (see TorrentListWindow's own fullScreen comment) and they
    // stack on top of each other; whichever had focus when the app was
    // last closed (see shutDown()) is brought to the front again, so
    // it's the one actually visible on startup — if that name no
    // longer matches anything (removed since, or a first run),
    // whichever ends up focused by default (the last one inserted)
    // stays that way.
    TorrentListWindow* toFocus = nullptr;
    for (const auto& [name, profile] : settings_.servers) {
        (void)profile; // only the name is needed here — openServerWindow() looks up the profile itself
        TorrentListWindow* win = openServerWindow(name);
        if (win && name == settings_.focusedServerAtClose) toFocus = win;
    }
    if (toFocus) toFocus->select();

    // Only meaningful now that every configured server's own window
    // actually exists and the right one (if any) has focus — the
    // placeholder initMenuBar() set up (a disabled "Empty") is what
    // shows until this first real rebuild.
    rebuildConnectionsMenu();
    rebuildPanelsMenu();
    if (TorrentListWindow* focused = focusedListWindow()) {
        lastConnectionsFocusedServer_ = focused->serverName();
    }
}

App::~App() {
    // NOT looping over allListWindows() to cancel any in-flight async
    // refresh here first — an earlier version of this did exactly that,
    // reasoning that every easy handle needs detaching from
    // multiHandle_ before curl_multi_cleanup() runs on it (still true —
    // see curl's own multi-handle docs). That reasoning was correct;
    // where it went wrong was assuming this destructor's own body runs
    // BEFORE the windows themselves are torn down. It doesn't: by the
    // time control reaches here, TApplication::run() has already called
    // shutDown() as part of its own normal exit path (see main.cpp) —
    // and TProgram::shutDown() (tprogram.cpp) sets deskTop = 0 before
    // TGroup::shutDown() actually destroys every child view, cascading
    // into each TorrentListWindow's own TransmissionClient destructor,
    // which ALREADY detaches safely from multiHandle_ if a refresh
    // happened to be in flight (see TransmissionClient's own destructor
    // comment) — the same safety net this loop was trying to provide
    // again, just redundantly, and via a deskTop pointer that's already
    // null by this point. Confirmed the hard way: a live crash
    // (AddressSanitizer SEGV) reading through that null pointer right
    // here, on every normal exit.
    if (multiHandle_) curl_multi_cleanup(multiHandle_);
}

TMenuBar* App::initMenuBar(TRect r) {
    r.b.y = r.a.y + 1;
    // Built separately and nested into the Torrent menu below via an
    // explicit cast to TMenuItem& — writing `*queueMenu + *new
    // TMenuItem(...)` inline there would resolve to operator+(TSubMenu&,
    // TSubMenu&) instead (see menus.h), which chains it as a sibling
    // TOP-LEVEL menu in the bar rather than nesting it as an item inside
    // Torrent's own list. The cast forces the other overload,
    // operator+(TSubMenu&, TMenuItem&) — a TSubMenu still qualifies
    // (it inherits from TMenuItem), just not the one the compiler picks
    // automatically. Documented once here since this project hasn't
    // needed a nested submenu before now.
    TSubMenu* queueMenu = new TSubMenu(tr(Str::MenuQueue), kbNoKey);
    *queueMenu +
        *new TMenuItem(tr(Str::MenuQueueMoveTop), cmQueueMoveTop, kbNoKey) +
        *new TMenuItem(tr(Str::MenuQueueMoveUp), cmQueueMoveUp, kbNoKey) +
        *new TMenuItem(tr(Str::MenuQueueMoveDown), cmQueueMoveDown, kbNoKey) +
        *new TMenuItem(tr(Str::MenuQueueMoveBottom), cmQueueMoveBottom, kbNoKey);

    // Same nested-submenu construction as queueMenu just above — see
    // its own comment for why the (TMenuItem&) cast below is needed.
    TSubMenu* priorityMenu = new TSubMenu(tr(Str::MenuPriority), kbNoKey);
    *priorityMenu +
        *new TMenuItem(tr(Str::MenuPriorityLow), cmSetPriorityLow, kbNoKey) +
        *new TMenuItem(tr(Str::MenuPriorityNormal), cmSetPriorityNormal, kbNoKey) +
        *new TMenuItem(tr(Str::MenuPriorityHigh), cmSetPriorityHigh, kbNoKey);

    // A single disabled placeholder to start — this runs before App's
    // own constructor has settings_ populated at all (see TProgInit's
    // own ordering, in the comment on this class in App.h), so the
    // real, per-server list only exists once rebuildConnectionsMenu()
    // runs for the first time, right after every configured server's
    // own window has been opened. g_connectionsMenu (file-scope, see
    // its own comment above) is how that later call finds this same
    // TMenu again — initMenuBar() has to be static (TProgInit's own
    // requirement), so there's no `this` yet to hand the pointer to
    // directly.
    TSubMenu* connectionsSubMenu = new TSubMenu(tr(Str::MenuConnectionsMenu), kbNoKey);
    *connectionsSubMenu + *new TMenuItem(tr(Str::MenuConnectionsEmpty), 0, kbNoKey);
    connectionsSubMenu->subMenu->items->disabled = True;
    g_connectionsMenu = connectionsSubMenu->subMenu;

    // Same placeholder-then-rebuild reasoning as connectionsSubMenu
    // above — rebuildPanelsMenu() (called once at startup, same as
    // rebuildConnectionsMenu()) replaces this with the real "Status"
    // item, bulleted or not depending on whether the window that ends
    // up focused first has its own panel open.
    TSubMenu* panelsSubMenu = new TSubMenu(tr(Str::MenuPanelsMenu), kbNoKey);
    *panelsSubMenu + *new TMenuItem(tr(Str::MenuPanelStatus), cmToggleStatusPanel, kbNoKey)
                   + *new TMenuItem(tr(Str::MenuPanelFiles), cmToggleFilesPanel, kbNoKey);
    g_panelsMenu = panelsSubMenu->subMenu;

    return new TMenuBar(r,
        *new TSubMenu(tr(Str::MenuTorrent), kbAltT) +
            *new TMenuItem(tr(Str::MenuAdd), cmAddTorrent, kbF2) +
            *new TMenuItem(tr(Str::MenuStart), cmStartTorrent, kbF5) +
            *new TMenuItem(tr(Str::MenuStop), cmStopTorrent, kbF6) +
            *new TMenuItem(tr(Str::MenuRemove), cmRemoveTorrent, kbF8) +
            *new TMenuItem(tr(Str::MenuDeleteWithData), cmDeleteTorrentWithData, kbNoKey) +
            newLine() +
            *new TMenuItem(tr(Str::MenuStartNow), cmStartNowTorrent, kbNoKey) +
            *new TMenuItem(tr(Str::MenuVerify), cmVerifyTorrent, kbNoKey) +
            *new TMenuItem(tr(Str::MenuReannounce), cmReannounceTorrent, kbNoKey) +
            *new TMenuItem(tr(Str::MenuShowDetails), cmShowDetails, kbNoKey) +
            *new TMenuItem(tr(Str::MenuShowFiles), cmShowFiles, kbNoKey) +
            newLine() +
            static_cast<TMenuItem&>(*queueMenu) +
            static_cast<TMenuItem&>(*priorityMenu) +
            newLine() +
            *new TMenuItem(tr(Str::MenuSelectMultiple), cmSelectMultiple, kbNoKey) +
            newLine() +
            *new TMenuItem(tr(Str::MenuQuit), cmQuit, kbAltX) +
        *connectionsSubMenu +
        *new TSubMenu(tr(Str::MenuColumnsMenu), kbNoKey) +
            // Rationalized from what used to be three separate entry
            // points here (a "Resize columns" submenu, an "Order
            // columns" submenu, and a standalone "Columns..." dialog —
            // each nested with the same (TMenuItem&) cast idiom that
            // was needed for those two submenus, no longer needed now
            // that there's only one plain item) into the single column
            // manager dialog — see ColumnManagerDialog.h.
            *new TMenuItem(tr(Str::MenuManageColumns), cmManageColumns, kbNoKey) +
        *new TSubMenu(tr(Str::MenuWindow), kbAltW) +
            // Standard tvision commands. Every torrent-list window
            // always exactly fills the desktop now (see
            // TorrentListWindow's own fullScreen comment) and stacks
            // rather than tiles alongside the others — Tile/Cascade
            // here only ever affects the "Torrent details"/files/
            // tracker windows, which are still ordinary, independently
            // sized and positioned MDI windows. Bringing a specific
            // server's own window to the front is what the
            // "Connections" menu (above) is for instead.
            *new TMenuItem(tr(Str::MenuWindowZoom), cmZoom, kbCtrlF5) +
            *new TMenuItem(tr(Str::MenuWindowNext), cmNext, kbCtrlF6) +
            *new TMenuItem(tr(Str::MenuWindowClose), cmClose, kbAltF3) +
            newLine() +
            *new TMenuItem(tr(Str::MenuWindowTile), cmTile, kbNoKey) +
            *new TMenuItem(tr(Str::MenuWindowCascade), cmCascade, kbNoKey) +
            newLine() +
            *new TMenuItem(tr(Str::MenuWindowList), cmShowWindowList, kbAlt0) +
            newLine() +
            // Nested WITHIN this Window submenu's own item chain, not a
            // new top-level menu-bar column of its own — needs the
            // explicit (TMenuItem&) cast, not just *panelsSubMenu, or
            // C++ overload resolution prefers TSubMenu&'s own operator+
            // (menus.h's own "chain two top-level submenus together"
            // overload) over TMenuItem&'s (menus.h's own "one more item
            // in the CURRENT chain" overload, which TSubMenu also
            // satisfies via its own TMenuItem base, just less directly)
            // — found by testing this live: without the cast, "Panels"
            // rendered as its own extra menu-bar entry next to "Window"
            // instead of inside it. Same idiom this project's own
            // comment on cmManageColumns above already named (for two
            // now-removed submenus that used to need it here too).
            (TMenuItem&)*panelsSubMenu +
        *new TSubMenu(tr(Str::MenuSettingsMenu), kbNoKey) +
            *new TMenuItem(tr(Str::MenuConnection), cmSettings, kbF9) +
            *new TMenuItem(tr(Str::MenuServerSettings), cmServerSettings, kbNoKey) +
            *new TMenuItem(tr(Str::MenuSessionStats), cmSessionStats, kbNoKey) +
        *new TSubMenu(tr(Str::MenuHelp), kbNoKey) +
            *new TMenuItem(tr(Str::MenuAbout), cmAbout, kbNoKey)
    );
}

TStatusLine* App::initStatusLine(TRect r) {
    r.a.y = r.b.y - 1;
    return new BandwidthStatusLine(r,
        *new TStatusDef(0, 0xFFFF) +
            // Text updated at runtime by updateBandwidthStatus(), from
            // whichever torrent-list window currently has focus — kbNoKey
            // because it's not an action, just information.
            *new TStatusItem("D: --  U: --", kbNoKey, cmBandwidthDisplay) +
            *new TStatusItem(tr(Str::StatusAdd), kbF2, cmAddTorrent) +
            *new TStatusItem(tr(Str::StatusStart), kbF5, cmStartTorrent) +
            *new TStatusItem(tr(Str::StatusStop), kbF6, cmStopTorrent) +
            *new TStatusItem(tr(Str::StatusSettings), kbF9, cmSettings) +
            *new TStatusItem(tr(Str::StatusQuit), kbAltX, cmQuit)
    );
}

TorrentListWindow* App::openServerWindow(const std::string& name) {
    // Already open: bring it forward instead of duplicating — this is
    // also how a server just added/edited in the Connection dialog
    // (see showConnectionDialog()) reaches an existing window rather
    // than always opening a new one.
    for (TorrentListWindow* w : allListWindows()) {
        if (w->serverName() == name) {
            w->select();
            // Same reasoning as the brand-new-window call further down
            // in this same function (see relayoutPanels()'s own
            // comment, TorrentListWindow.h) — found directly still
            // needed here too, separately: a window created once at
            // startup, covered by whichever other one was focused at
            // the time, then brought to the front later by picking it
            // from the Connections menu (this exact branch) rather
            // than being freshly created, was found with neither its
            // own open- nor closed-state collapse arrow ever actually
            // showing, on either side — select() alone (bringing a
            // window forward) doesn't ask it to redraw itself the way
            // an explicit call here does.
            w->relayoutPanels();
            return w;
        }
    }

    auto profileIt = settings_.servers.find(name);
    if (profileIt == settings_.servers.end()) return nullptr; // stays defensive; callers only ever pass a name that's actually in settings_.servers
    const ServerProfile& profile = profileIt->second;

    // Each window gets its own TransmissionClient rather than sharing
    // one — see App.h's own comment on clients_ for why a std::map
    // specifically. The reference TorrentListWindow holds (client_ in
    // its own header) stays valid for as long as this entry does, which
    // is until the corresponding server is removed via the Connection
    // dialog's "[-]" (see showConnectionDialog()) — never while its own
    // window still exists.
    auto client = std::make_unique<TransmissionClient>(profile.host, profile.port,
                                                         profile.user, profile.password,
                                                         profile.rpcPath);
    TransmissionClient& clientRef = *client;
    clients_[name] = std::move(client);

    // This server's own column layout, if it's ever been customized —
    // a default-constructed ColumnLayout (empty vectors) otherwise,
    // which TorrentListWindow's own constructor already treats as "use
    // the built-in defaults" (see AppSettings::ColumnLayout's own
    // comment on why this is per-server now, not shared).
    AppSettings::ColumnLayout columnLayout;
    auto layoutIt = settings_.columnLayouts.find(name);
    if (layoutIt != settings_.columnLayouts.end()) columnLayout = layoutIt->second;

    auto* win = new TorrentListWindow(deskTop->getExtent(), name, clientRef,
        settings_.sortColumn, settings_.sortAscending, settings_.filter,
        columnLayout.widths, columnLayout.order, columnLayout.visible,
        [this](SortColumn col, bool asc) {
            settings_.sortColumn = col;
            settings_.sortAscending = asc;
            saveSettings(settings_);
        },
        // `filter` is global (see AppSettings::filter's own comment) —
        // applied to every currently open window, not just whichever
        // one's own Status panel this change actually came from, the
        // same "one setting, every window" behavior the old modal
        // Filters dialog this panel replaced already had (see "Fixed
        // bugs" in README.md).
        [this](const TorrentFilter& f) {
            settings_.filter = f;
            saveSettings(settings_);
            for (TorrentListWindow* w : allListWindows()) w->setFilter(f);
        },
        settings_.trackerColumnWidths, settings_.trackerColumnOrder, settings_.trackerColumnVisible,
        settings_.peerColumnWidths, settings_.peerColumnOrder, settings_.peerColumnVisible);
    deskTop->insert(win); // TorrentListWindow's own constructor already calls refresh() at the end — nothing more needed here
    // Unconditional — not gated on whether a saved PanelLayout below
    // actually opens a panel. See relayoutPanels()'s own comment
    // (TorrentListWindow.h) for why this specific spot (after
    // insert(), not inside the constructor) is what makes this work at
    // all — reported directly, with neither of this window's own
    // collapse/expand arrows showing at all for a server whose panels
    // had never been toggled.
    win->relayoutPanels();

    // This server's own saved panel state, if it's ever had one opened
    // (a default-constructed PanelLayout — every panel closed — falls
    // out of find() failing, same "never customized yet" fallback
    // columnLayout above uses).
    auto panelIt = settings_.panelLayouts.find(name);
    if (panelIt != settings_.panelLayouts.end() && panelIt->second.statusOpen) {
        win->setStatusPanelOpen(true, panelIt->second.statusWidth);
    }
    if (panelIt != settings_.panelLayouts.end() && panelIt->second.filesOpen) {
        win->setFilesPanelOpen(true, panelIt->second.filesWidth);
    }
    return win;
}

void App::showAddTorrentDialog(const std::string& initialValue, const std::string& initialDestination) {
    // Adds to whichever server's window currently has focus — the same
    // "act on the focused one" rule every other Torrent-menu command
    // follows now that there's more than one to choose from.
    TorrentListWindow* target = focusedListWindow();
    if (!target) return;
    auto clientIt = clients_.find(target->serverName());
    if (clientIt == clients_.end()) return;
    TransmissionClient& client = *clientIt->second;

    TInputLine* urlField = nullptr;
    auto* dlg = createAddTorrentDialog(urlField, client, initialValue, initialDestination);
    if (!dlg) return;
    ushort result = execView(dlg);
    // Captured regardless of which command ended the dialog — Browse
    // and Change... both reopen this same dialog afterward (see their
    // own branches below), and whatever the user had already typed/
    // chosen needs to survive that round trip either way, not just on
    // a genuine cmOK.
    std::string url = addTorrentDialogResult(urlField);
    std::string destination = addTorrentDialogDestination(dlg);
    destroy(dlg);

    if (result == cmOK) {
        if (!url.empty()) {
            auto addResult = client.addTorrent(url, destination);
            if (addResult == TransmissionClient::AddTorrentResult::Duplicate) {
                messageBox(tr(Str::MsgTorrentDuplicate), mfInformation | mfOKButton);
            } else if (addResult == TransmissionClient::AddTorrentResult::Failed) {
                // lastError() carries Transmission's own reason when the
                // RPC request itself succeeded but adding the torrent
                // didn't (invalid/corrupt magnet or .torrent, unreachable
                // http(s) URL, ...), or a network/curl error when the
                // request couldn't even be made — either way, something
                // concrete to show instead of doing nothing.
                char buf[512];
                std::snprintf(buf, sizeof(buf), tr(Str::MsgTorrentAddFailed),
                    client.lastError().c_str());
                messageBox(buf, mfError | mfOKButton);
            }
        }
        return;
    }

    if (result == cmYes) {
        // Browse was clicked. The "Add torrent" dialog above is already
        // destroyed at this point — deliberately, before opening
        // TFileDialog: nesting TFileDialog *inside* an already-open
        // dialog (as a "Browse" button used to do) rendered with wrong
        // colors and garbled text (fragments of both dialogs bleeding
        // into each other, readable in a screenshot the user sent).
        // Rebuilding a whole separate custom directory-browser dialog
        // to sidestep that turned out to be unnecessary once the real
        // cause was found: it wasn't TFileDialog's fault specifically
        // (the same custom replacement showed the identical corruption)
        // — it was two dialogs being modal at once. Closing this one
        // first, THEN opening TFileDialog directly from `this` (one
        // level of nesting, exactly like every other dialog in this
        // app, including "Add torrent" itself), avoids that entirely —
        // simpler than maintaining a hand-built browser. The SAME
        // reasoning is why "Change..." below opens TFolderBrowserDialog
        // the same way, rather than nesting that inside this dialog
        // either.
        auto* fileDlg = new TFileDialog("*.torrent", tr(Str::DialogTitleBrowseTorrent),
            tr(Str::LabelAddTorrentUrl), fdOpenButton, 0);
        ushort fileResult = execView(fileDlg);
        std::string chosenPath;
        // The "Open" button's command is cmFileOpen, not cmOK (checked
        // in tvision's tfildlg.cpp) — only double-clicking a file in the
        // list re-emits as cmOK. Either one is a real selection; cmCancel
        // is the only "nothing chosen" case.
        if (fileResult == cmFileOpen || fileResult == cmOK) {
            char buf[1024] = {0};
            fileDlg->getFileName(buf);
            chosenPath = buf;
        }
        destroy(fileDlg);

        // Reopen with whatever was picked pre-filled — Browse fills the
        // field, it doesn't add the torrent by itself; the user still
        // confirms (or edits further, or cancels) from here. The
        // destination chosen before Browse was clicked carries over
        // unchanged (Browse only ever affects the URL/file field).
        showAddTorrentDialog(chosenPath.empty() ? url : chosenPath, destination);
        return;
    }

    if (result == cmChangeFolder) {
        // "Change..." (destination folder) was clicked — same "close
        // first, one level of nesting" reasoning as Browse just above.
        // Labels built from this app's own translated strings right
        // here rather than baked into TFolderBrowserDialog itself,
        // which has no translation system of its own to draw on (see
        // its own header comment) — the same pattern already used for
        // TGridColumnManagerDialog's own labels.
        TFolderBrowserLabels labels;
        labels.title = tr(Str::DialogTitleSelectFolder);
        labels.pathLabel = tr(Str::LabelFolderPath);
        labels.selectButton = tr(Str::ButtonSelect);
        labels.cancelButton = tr(Str::ButtonCancel);
        labels.unreadableDirectory = tr(Str::MsgFolderUnreadable);
        auto* folderDlg = createFolderBrowserDialog(destination, labels);
        ushort folderResult = execView(folderDlg);
        std::string chosenFolder = (folderResult == cmOK) ? folderBrowserResult(folderDlg) : destination;
        destroy(folderDlg);

        // Reopens with the URL/file field exactly as it was — "Change..."
        // only ever affects the destination.
        showAddTorrentDialog(url, chosenFolder);
    }
}

void App::showConnectionDialog() {
    ConnectionDialogFields fields;
    // Runs the INSTANT "[-]" actually removes a server — not gated
    // behind this dialog's own OK/Cancel at all (see
    // ServerRemovedCallback's own doc comment in ConnectionDialog.h for
    // why waiting for either would leave stale windows behind). Closes
    // every window still pointing at that server's client — its own
    // torrent-list window, and any Details/Files/Tracker window opened
    // from it for one of its torrents — before dropping the server
    // from settings_ and persisting that right away, so a Cancel right
    // after doesn't quietly undo a removal its own confirmation popup
    // already told the user had happened.
    auto onServerRemoved = [this](const std::string& name) {
        auto clientIt = clients_.find(name);
        if (clientIt != clients_.end()) {
            // Same ordering as ever: other windows first, while the
            // client identifying them is still alive, then the
            // torrent-list window, and only then the client itself.
            closeWindowsForClient(clientIt->second.get());
            for (TorrentListWindow* w : allListWindows()) {
                if (w->serverName() == name) { w->close(); break; }
            }
            clients_.erase(clientIt);
        }
        settings_.servers.erase(name);
        if (settings_.activeServer == name) settings_.activeServer.clear();
        saveSettings(settings_);

        // The server list itself just changed — rebuilt right here
        // rather than waiting for idle()'s own focus-change check (see
        // its own comment), which wouldn't notice this on its own if
        // focus happens to already be on some OTHER window.
        rebuildConnectionsMenu();
        if (TorrentListWindow* focused = focusedListWindow()) {
            lastConnectionsFocusedServer_ = focused->serverName();
        } else {
            lastConnectionsFocusedServer_.clear();
        }
    };

    // Runs the moment Save/OK actually tests a connection successfully
    // (see ServerSavedCallback's own doc comment in ConnectionDialog.h)
    // — persists that server's profile and opens (or updates) its own
    // window right here, not deferred to whenever/if this dialog
    // eventually closes with cmOK: Save deliberately keeps the dialog
    // open (see the button's own comment), so waiting for cmOK the way
    // this used to would mean nothing happens at all unless the user
    // also clicks the now-relabeled "OK" afterward.
    auto onServerSaved = [this](const std::string& name, const ServerProfile& profile) {
        settings_.servers[name] = profile;
        settings_.activeServer = name;
        saveSettings(settings_);

        TorrentListWindow* target = nullptr;
        for (TorrentListWindow* w : allListWindows()) {
            if (w->serverName() == name) { target = w; break; }
        }
        if (target) {
            auto clientIt = clients_.find(name);
            if (clientIt != clients_.end()) {
                clientIt->second->setEndpoint(profile.host, profile.port);
                clientIt->second->setCredentials(profile.user, profile.password);
                clientIt->second->setRpcPath(profile.rpcPath);
                target->refresh();
            }
            target->select();
        } else {
            openServerWindow(name);
        }

        // Same reasoning as onServerRemoved's own rebuild above: the
        // server list (a brand new name) or which one has focus (an
        // existing one just brought forward) may have just changed,
        // and idle()'s own check shouldn't be the only thing that
        // eventually notices.
        rebuildConnectionsMenu();
        if (TorrentListWindow* focused = focusedListWindow()) {
            lastConnectionsFocusedServer_ = focused->serverName();
        }
    };

    if (auto* dlg = createConnectionDialog(settings_, fields, onServerRemoved, onServerSaved)) {
        if (execView(dlg) == cmOK) {
            // Only refreshInterval/language left to apply here — by the
            // time this dialog can even close with cmOK, the button
            // read "OK" (not "Save"), meaning whichever server was last
            // shown was already fully saved and its window already
            // opened/updated by onServerSaved above. Nothing further to
            // do for server data specifically.
            Language oldLanguage = settings_.language;
            settings_ = connectionDialogResult(fields, settings_);
            saveSettings(settings_); // persisted right away: see Config.h
            setLanguage(settings_.language);

            // The menu bar and status bar, on the other hand, are built
            // only once at startup (see main.cpp/App.h) and stay in
            // whatever language was active then until the app is
            // restarted — but from this point on restarting *works*:
            // the config file now holds the chosen language. Every
            // still-open torrent-list window gets relabeled right away.
            for (TorrentListWindow* w : allListWindows()) w->retranslate();

            // Told explicitly rather than left to notice on their own:
            // most of the UI already switched (see retranslate() above
            // and every other window/dialog, rebuilt fresh each time
            // it's shown), so a restart looks unnecessary until they
            // spot the still-old menu bar/status bar.
            if (settings_.language != oldLanguage) {
                messageBox(tr(Str::MsgLanguageChangeRestart), mfInformation | mfOKButton);
            }
        }
        destroy(dlg);
    }
}

void App::showServerSettingsDialog() {
    // Acts on whichever server's window currently has focus — global
    // (session-wide) speed limits are the connected Transmission
    // daemon's OWN state, and with more than one daemon possibly
    // connected to at once now, "the" limits only makes sense per
    // server, the same way "Manage columns..." already acts on
    // whichever grid has focus (see focusedGrid()).
    TorrentListWindow* focused = focusedListWindow();
    if (!focused) return;
    auto clientIt = clients_.find(focused->serverName());
    if (clientIt == clients_.end()) return;
    TransmissionClient& client = *clientIt->second;

    // Live RPC call: none of these fields are part of settings_ /
    // settings.json, they live on the Transmission daemon itself (see
    // TransmissionClient::getSessionLimits()). If it doesn't work (e.g.
    // this server's own host/user/password were never set up
    // successfully), the fetch fails and `sessionLimitsFetched` says so.
    bool sessionLimitsFetched = false;
    SessionLimits sessionLimits = client.getSessionLimits(&sessionLimitsFetched);

    ServerSettingsDialogFields fields;
    if (auto* dlg = createServerSettingsDialog(sessionLimits, fields, client)) {
        if (execView(dlg) == cmOK) {
            // Only pushed back if the fetch above actually succeeded.
            // Otherwise the dialog's fields were showing meaningless
            // defaults (0/disabled) rather than this server's real
            // state, most commonly when the connection isn't working
            // yet — sending those defaults would silently wipe out real
            // limits already set there, even though the user never
            // touched any of these fields.
            if (sessionLimitsFetched) {
                client.setSessionLimits(serverSettingsDialogResult(fields));
            }
        }
        destroy(dlg);
    }
}

void App::showSessionStatsDialog() {
    // Same "acts on whichever server's window currently has focus"
    // reasoning as showServerSettingsDialog() just above — session
    // stats are the connected daemon's own state too.
    TorrentListWindow* focused = focusedListWindow();
    if (!focused) return;
    auto clientIt = clients_.find(focused->serverName());
    if (clientIt == clients_.end()) return;
    TransmissionClient& client = *clientIt->second;

    // A failed initial fetch still opens the dialog (showing all
    // zeros) rather than silently doing nothing — its own "Refresh"
    // button gives an easy way to retry without reopening it, and
    // there's no "push changes back" step here (unlike Server Settings)
    // that a failed fetch would need to guard against.
    SessionStats stats = client.getSessionStats();
    if (auto* dlg = createSessionStatsDialog(stats, client)) {
        execView(dlg);
        destroy(dlg);
    }
}

void App::toggleStatusPanelForFocused() {
    TorrentListWindow* target = focusedListWindow();
    if (!target) return;
    bool nowOpen = !target->isStatusPanelOpen();
    // The width to open at: whatever this SAME server's own panel was
    // last dragged to (see AppSettings::PanelLayout), not always the
    // built-in default — closing it doesn't forget that, only whether
    // it's shown.
    auto& layout = settings_.panelLayouts[target->serverName()]; // creates a default entry if none yet
    int width = layout.statusWidth;
    target->setStatusPanelOpen(nowOpen, width);
    layout.statusOpen = nowOpen;
    if (!nowOpen) layout.statusWidth = target->statusPanelWidth(); // remembers a resize even while now closed
    saveSettings(settings_);
    rebuildPanelsMenu();
}

void App::toggleFilesPanelForFocused() {
    TorrentListWindow* target = focusedListWindow();
    if (!target) return;
    bool nowOpen = !target->isFilesPanelOpen();
    auto& layout = settings_.panelLayouts[target->serverName()];
    int width = layout.filesWidth;
    target->setFilesPanelOpen(nowOpen, width);
    layout.filesOpen = nowOpen;
    if (!nowOpen) layout.filesWidth = target->filesPanelWidth();
    saveSettings(settings_);
    rebuildPanelsMenu();
}

void App::resizeStatusPanelForFocused() {
    TorrentListWindow* target = focusedListWindow();
    if (!target || !target->isStatusPanelOpen()) return;
    target->keyboardResizeStatusPanel();
    // Same persistence as a mouse drag — the keyboard mode itself
    // doesn't know about settings_/serverName() at all (a
    // TorrentListWindow concern only), so the width is read back and
    // saved here once the mode returns, exactly as it would be after
    // a drag confirms.
    settings_.panelLayouts[target->serverName()].statusWidth = target->statusPanelWidth();
    saveSettings(settings_);
}

void App::resizeFilesPanelForFocused() {
    TorrentListWindow* target = focusedListWindow();
    if (!target || !target->isFilesPanelOpen()) return;
    target->keyboardResizeFilesPanel();
    settings_.panelLayouts[target->serverName()].filesWidth = target->filesPanelWidth();
    saveSettings(settings_);
}

void App::showColumnManagerDialog() {
    // Acts on whichever window currently has focus — see focusedGrid()
    // and idle() (which keeps the menu item itself disabled whenever
    // this would come back null, so reaching here with none is only a
    // defensive fallback against a focus change slipping in between the
    // command firing and this running).
    TGridView* grid = focusedGrid();
    if (!grid) return;

    // This dialog lives in tvision-ext/ (see its own TGridView-README.md) and has
    // no dependency on this app's tr()/Str translation system — so its
    // text is built here, once, from what this app already has
    // translated, rather than the dialog knowing anything about
    // languages at all.
    TGridColumnManagerLabels labels;
    labels.title = tr(Str::DialogTitleColumnManager);
    labels.columnHeader = tr(Str::LabelColumnManagerColumn);
    labels.widthHeader = tr(Str::LabelColumnManagerWidth);
    labels.visibleHeader = tr(Str::LabelColumnManagerVisible);
    // labels.yes/labels.no left at their defaults ("[X]"/"[ ]") — a
    // checkbox glyph doesn't need translating the way the rest of this
    // does.
    labels.resizeButton = tr(Str::ButtonResizeColumn);
    labels.moveButton = tr(Str::ButtonMoveColumn);
    labels.toggleVisibleButton = tr(Str::ButtonToggleVisible);
    labels.resetButton = tr(Str::ButtonReset);
    labels.closeButton = tr(Str::ButtonClose);

    if (auto* dlg = createColumnManagerDialog(grid, labels)) {
        // Unlike the Filters/Settings dialogs, there's nothing to read
        // back from this one on close: every action inside it (resize,
        // move, toggle visible) applies straight to the grid as it
        // happens — see TGridColumnManagerDialog.h's own doc comment
        // for why.
        execView(dlg);
        destroy(dlg);
        // Forces `grid` to redraw now that the dialog covering it is
        // actually gone — belt-and-suspenders alongside TGridView's own
        // draw()-time self-correction (see its header comment on
        // updateHScrollBarVisibility()) for the same reason: whatever
        // changed while covered (a column shown/hidden, most commonly)
        // should be reflected the instant this window is visible again,
        // not only whenever its next unrelated redraw happens to occur.
        grid->refresh();
        // Persisted here as a natural "done editing" point, same
        // reasoning as App::shutDown() persisting these on exit — the
        // user might not close the app again for a while after this.
        // Which settings.json fields to update depends on which grid
        // was actually just edited — checked by the focused window's
        // own class: a torrent-list window saves under its own server
        // name (see AppSettings::ColumnLayout's own comment on why
        // per-server, not shared, now that each server has its own
        // independently-managed MDI window), the tracker list under
        // its own separate, still-shared layout.
        if (auto* listWin = dynamic_cast<TorrentListWindow*>(TProgram::deskTop->current)) {
            AppSettings::ColumnLayout layout;
            layout.widths = listWin->columnWidths();
            layout.order = listWin->columnOrder();
            layout.visible = listWin->columnVisibility();
            settings_.columnLayouts[listWin->serverName()] = layout;
            saveSettings(settings_);
        } else if (auto* trackerWin = dynamic_cast<TrackerPeerWindow*>(TProgram::deskTop->current)) {
            // Which pair of AppSettings fields depends on which tab was
            // actually showing when "Manage columns..." was used —
            // columnWidths()/columnOrder()/columnVisibility() all
            // reflect whichever one that was (see TrackerPeerWindow's
            // own doc comment on isPeersTabActive()), not necessarily
            // the Trackers tab just because that's the one shown first.
            if (trackerWin->isPeersTabActive()) {
                settings_.peerColumnWidths = trackerWin->columnWidths();
                settings_.peerColumnOrder = trackerWin->columnOrder();
                settings_.peerColumnVisible = trackerWin->columnVisibility();
            } else {
                settings_.trackerColumnWidths = trackerWin->columnWidths();
                settings_.trackerColumnOrder = trackerWin->columnOrder();
                settings_.trackerColumnVisible = trackerWin->columnVisibility();
            }
            saveSettings(settings_);
        }
    }
}

void App::showWindowListDialog() {
    // deskTop->last/next: TGroup's public circular chain, the same
    // traversal mechanism already used elsewhere in this project. Order
    // isn't an issue here (unlike the SettingsDialog field bug): we're
    // just listing windows to choose from, not remapping values by index.
    std::vector<TWindow*> windows;
    if (deskTop->last) {
        TView* p = deskTop->last;
        do {
            p = p->next;
            if (auto* w = dynamic_cast<TWindow*>(p))
                windows.push_back(w);
        } while (p != deskTop->last);
    }
    if (windows.empty()) return;

    WindowListViewer* viewer = nullptr;
    if (auto* dlg = createWindowListDialog(std::move(windows), viewer)) {
        if (execView(dlg) == cmOK) {
            if (TWindow* selected = viewer->selectedWindow())
                selected->select(); // brings it to the front and focuses it
        }
        destroy(dlg);
    }
}

void App::showAboutDialog() {
    if (auto* dlg = createAboutDialog()) {
        execView(dlg);
        destroy(dlg);
    }
}

void App::updateBandwidthStatus() {
    // Whichever torrent-list window has focus, per the decision that
    // this reflects "the open window", not every open window's combined
    // total — shows the placeholder "--"/"--" (see initStatusLine())
    // when no torrent-list window has focus at all (some other kind of
    // window does, or none does).
    if (!statusLine) return;
    TorrentListWindow* focused = focusedListWindow();
    char buf[64];
    if (focused) {
        double down = focused->totalDownloadRate();
        double up = focused->totalUploadRate();
        std::snprintf(buf, sizeof(buf), "D: %.1f KB/s  U: %.1f KB/s", down / 1024.0, up / 1024.0);
    } else {
        std::snprintf(buf, sizeof(buf), "D: --  U: --");
    }
    static_cast<BandwidthStatusLine*>(statusLine)->setItemText(cmBandwidthDisplay, buf);
}

TGridView* App::focusedGrid() const {
    // deskTop->current is the currently active window. Every
    // TGridView-based window in this app — TorrentListWindow (via its
    // TGridWindow base), TrackerPeerWindow, TorrentFilesWindow —
    // inserts its grid as a direct child, the same way any TView is
    // inserted into its owning TGroup, so one plain child search covers
    // all of them without needing to know which specific window class
    // it is.
    TView* focused = TProgram::deskTop->current;
    if (!focused) return nullptr;
    auto* group = dynamic_cast<TGroup*>(focused);
    if (!group) return nullptr;
    TGridView* grid = nullptr;
    group->forEach([](TView* v, void* arg) {
        if (auto* g = dynamic_cast<TGridView*>(v)) *(TGridView**)arg = g;
    }, &grid);
    return grid;
}

TorrentListWindow* App::focusedListWindow() const {
    return dynamic_cast<TorrentListWindow*>(TProgram::deskTop->current);
}

std::vector<TorrentListWindow*> App::allListWindows() const {
    std::vector<TorrentListWindow*> result;
    if (TProgram::deskTop->last) {
        TView* p = TProgram::deskTop->last;
        do {
            p = p->next;
            if (auto* w = dynamic_cast<TorrentListWindow*>(p)) result.push_back(w);
        } while (p != TProgram::deskTop->last);
    }
    return result;
}

void App::rebuildConnectionsMenu() {
    if (!g_connectionsMenu) return; // shouldn't happen — initMenuBar() always sets it — stays defensive

    // Frees the OLD item chain the same way TMenu::~TMenu() itself
    // would (see tvision's own menu.cpp) — without destroying the TMenu
    // object itself, since this is the one TMenu ever found again (see
    // g_connectionsMenu's own comment) and gets reused in place rather
    // than replaced.
    TMenuItem* p = g_connectionsMenu->items;
    while (p != nullptr) {
        TMenuItem* next = p->next;
        delete p;
        p = next;
    }
    g_connectionsMenu->items = nullptr;
    g_connectionsMenu->deflt = nullptr;

    if (settings_.servers.empty()) {
        TMenuItem* empty = new TMenuItem(tr(Str::MenuConnectionsEmpty), 0, kbNoKey);
        empty->disabled = True;
        g_connectionsMenu->items = empty;
        g_connectionsMenu->deflt = empty;
        return;
    }

    std::string focusedName;
    if (TorrentListWindow* focused = focusedListWindow()) focusedName = focused->serverName();

    TMenuItem* head = nullptr;
    TMenuItem* tail = nullptr;
    ushort cmd = cmConnectionBase;
    for (const auto& [name, profile] : settings_.servers) {
        (void)profile;
        // A bullet, and the whole label wrapped in "~...~" — normally
        // how a menu item marks just its own single accelerator letter
        // for underlining, repurposed here to color the ENTIRE label in
        // that same highlight color instead of just one letter: tvision
        // has no per-item bold/font-weight of its own in a text-mode
        // menu (every item in the same state shares one color, computed
        // once for the whole menu, not per item — see TMenuBox::draw()),
        // so this is the closest thing to "make this one item visually
        // stand out" without writing a custom menu-drawing class just
        // for it.
        std::string label = (name == focusedName)
            ? ("\xE2\x97\x8F ~" + name + "~") // U+25CF BLACK CIRCLE, UTF-8
            : ("  " + name);
        TMenuItem* item = new TMenuItem(label.c_str(), cmd, kbNoKey);
        if (head == nullptr) head = item; else tail->next = item;
        tail = item;
        cmd++;
    }
    g_connectionsMenu->items = head;
    g_connectionsMenu->deflt = head;
}

void App::rebuildPanelsMenu() {
    if (!g_panelsMenu) return; // shouldn't happen — initMenuBar() always sets it — stays defensive

    TMenuItem* p = g_panelsMenu->items;
    while (p != nullptr) {
        TMenuItem* next = p->next;
        delete p;
        p = next;
    }

    // Same "no per-item bold/font-weight, so a bullet + ~...~ wrapping
    // the whole label is the closest substitute" reasoning as
    // rebuildConnectionsMenu() above — a focused window with no panel
    // open at all (or no window focused yet, e.g. before any server's
    // configured) shows the plain, unbulleted label, same as any
    // server that isn't the focused one over there.
    TorrentListWindow* focused = focusedListWindow();
    bool statusOpen = focused && focused->isStatusPanelOpen();
    bool filesOpen = focused && focused->isFilesPanelOpen();

    // Shared by both items below: tr()'s own hotkey markup ("~S~tatus")
    // underlines just one letter, which the bulleted case replaces with
    // wrapping the WHOLE label instead — the two styles can't just be
    // nested, so every "~" is stripped first either way.
    auto buildLabel = [](Str id, bool open) {
        std::string plain = tr(id);
        std::string noMarkup;
        for (char c : plain) if (c != '~') noMarkup += c;
        return open ? ("\xE2\x97\x8F ~" + noMarkup + "~") : plain;
    };

    TMenuItem* statusItem = new TMenuItem(buildLabel(Str::MenuPanelStatus, statusOpen).c_str(),
                                           cmToggleStatusPanel, kbNoKey);
    TMenuItem* filesItem = new TMenuItem(buildLabel(Str::MenuPanelFiles, filesOpen).c_str(),
                                          cmToggleFilesPanel, kbNoKey);
    TMenuItem* tail = filesItem;
    // "Resize ...": only present once the matching panel is actually
    // open — resizing a closed panel isn't a meaningful action, so
    // there's nothing useful to show for it rather than a disabled
    // item explaining why.
    if (statusOpen) {
        tail->next = new TMenuItem(tr(Str::MenuPanelResizeStatus), cmResizeStatusPanel, kbNoKey);
        tail = tail->next;
    }
    if (filesOpen) {
        tail->next = new TMenuItem(tr(Str::MenuPanelResizeFiles), cmResizeFilesPanel, kbNoKey);
        tail = tail->next;
    }
    statusItem->next = filesItem;
    g_panelsMenu->items = statusItem;
    g_panelsMenu->deflt = statusItem;
}

void App::closeWindowsForClient(TransmissionClient* client) const {
    // Collected first, closed after: TWindow::close() calls destroy(this),
    // which would mutate deskTop's own child chain out from under this
    // same walk if done while still iterating it.
    std::vector<TWindow*> toClose;
    if (TProgram::deskTop->last) {
        TView* p = TProgram::deskTop->last;
        do {
            p = p->next;
            if (auto* w = dynamic_cast<TorrentDetailsWindow*>(p)) {
                if (w->clientPtr() == client) toClose.push_back(w);
            } else if (auto* w = dynamic_cast<TorrentFilesWindow*>(p)) {
                if (w->clientPtr() == client) toClose.push_back(w);
            } else if (auto* w = dynamic_cast<TrackerPeerWindow*>(p)) {
                if (w->clientPtr() == client) toClose.push_back(w);
            }
        } while (p != TProgram::deskTop->last);
    }
    for (TWindow* w : toClose) w->close();
}

void App::handleEvent(TEvent& event) {
    TApplication::handleEvent(event);
    if (event.what != evCommand) return;

    // The "Connections" menu's own dynamic per-server commands (see
    // rebuildConnectionsMenu()) aren't compile-time constants a switch
    // could have a case label for — checked as a range here instead,
    // ahead of it. Which server a given command corresponds to is
    // recomputed the same way it was assigned when the menu was built:
    // walking settings_.servers (a std::map, so always the same
    // alphabetical order) exactly `index` steps in.
    if (event.message.command >= cmConnectionBase &&
        event.message.command < cmConnectionBase + (ushort)settings_.servers.size()) {
        int index = event.message.command - cmConnectionBase;
        auto it = settings_.servers.begin();
        std::advance(it, index);
        openServerWindow(it->first); // already open — just brings it to the front (see its own comment)
        clearEvent(event);
        return;
    }

    switch (event.message.command) {
        case cmAddTorrent:
            showAddTorrentDialog();
            clearEvent(event);
            break;
        case cmStartTorrent:
            if (auto* w = focusedListWindow()) w->startSelected();
            clearEvent(event);
            break;
        case cmStopTorrent:
            if (auto* w = focusedListWindow()) w->stopSelected();
            clearEvent(event);
            break;
        case cmRemoveTorrent:
            if (auto* w = focusedListWindow()) w->removeSelected();
            clearEvent(event);
            break;
        case cmDeleteTorrentWithData:
            if (auto* w = focusedListWindow()) w->deleteWithDataSelected();
            clearEvent(event);
            break;
        case cmStartNowTorrent:
            if (auto* w = focusedListWindow()) w->startNowSelected();
            clearEvent(event);
            break;
        case cmVerifyTorrent:
            if (auto* w = focusedListWindow()) w->verifySelected();
            clearEvent(event);
            break;
        case cmReannounceTorrent:
            if (auto* w = focusedListWindow()) w->reannounceSelected();
            clearEvent(event);
            break;
        case cmShowDetails:
            if (auto* w = focusedListWindow()) w->showDetailsForSelected();
            clearEvent(event);
            break;
        case cmShowFiles:
            if (auto* w = focusedListWindow()) w->showFilesForSelected();
            clearEvent(event);
            break;
        case cmSelectMultiple:
            // A toggle, not a one-way "enter": the same menu item exits
            // selection mode again if it's already active — enterSelectionMode()/
            // exitSelectionMode() are both already safe no-ops in the
            // wrong state, so there's nothing extra to guard here.
            if (auto* w = focusedListWindow()) {
                if (w->grid()->isInSelectionMode())
                    w->grid()->exitSelectionMode();
                else
                    w->grid()->enterSelectionMode(w->grid()->focusedRow());
            }
            clearEvent(event);
            break;
        case cmQueueMoveTop:
            if (auto* w = focusedListWindow()) w->queueMoveTopForSelected();
            clearEvent(event);
            break;
        case cmQueueMoveUp:
            if (auto* w = focusedListWindow()) w->queueMoveUpForSelected();
            clearEvent(event);
            break;
        case cmQueueMoveDown:
            if (auto* w = focusedListWindow()) w->queueMoveDownForSelected();
            clearEvent(event);
            break;
        case cmQueueMoveBottom:
            if (auto* w = focusedListWindow()) w->queueMoveBottomForSelected();
            clearEvent(event);
            break;
        case cmSetPriorityLow:
            if (auto* w = focusedListWindow()) w->setPriorityForSelected(-1);
            clearEvent(event);
            break;
        case cmSetPriorityNormal:
            if (auto* w = focusedListWindow()) w->setPriorityForSelected(0);
            clearEvent(event);
            break;
        case cmSetPriorityHigh:
            if (auto* w = focusedListWindow()) w->setPriorityForSelected(1);
            clearEvent(event);
            break;
        case cmSettings:
            showConnectionDialog();
            clearEvent(event);
            break;
        case cmServerSettings:
            showServerSettingsDialog();
            clearEvent(event);
            break;
        case cmSessionStats:
            showSessionStatsDialog();
            clearEvent(event);
            break;
        case cmToggleStatusPanel:
            toggleStatusPanelForFocused();
            clearEvent(event);
            break;
        case cmToggleFilesPanel:
            toggleFilesPanelForFocused();
            clearEvent(event);
            break;
        case cmResizeStatusPanel:
            resizeStatusPanelForFocused();
            clearEvent(event);
            break;
        case cmResizeFilesPanel:
            resizeFilesPanelForFocused();
            clearEvent(event);
            break;
        case cmManageColumns:
            showColumnManagerDialog();
            clearEvent(event);
            break;
        case cmShowWindowList:
            showWindowListDialog();
            clearEvent(event);
            break;
        case cmAbout:
            showAboutDialog();
            clearEvent(event);
            break;
        case cmBandwidthDisplay:
            // Purely informational item: a click should do nothing.
            clearEvent(event);
            break;
        default:
            break;
    }
}

void App::shutDown() {
    // Every still-open window's own column layout (widths/order/
    // visibility together — see AppSettings::ColumnLayout's own
    // comment on why per-server now, not shared), keyed by its server
    // name — rebuilt from scratch each time (not just updated in
    // place): every configured server always has its own open window
    // (the MDI invariant this app maintains — see its own constructor),
    // so rebuilding from allListWindows() can't lose a legitimate one,
    // and a server removed via the Connection dialog during this
    // session doesn't leave a stale entry behind either.
    std::vector<TorrentListWindow*> windows = allListWindows();
    settings_.columnLayouts.clear();
    for (TorrentListWindow* w : windows) {
        AppSettings::ColumnLayout layout;
        layout.widths = w->columnWidths();
        layout.order = w->columnOrder();
        layout.visible = w->columnVisibility();
        settings_.columnLayouts[w->serverName()] = layout;
    }

    // Same "rebuilt from scratch, not just updated in place" reasoning
    // as columnLayouts just above — every currently open window's own
    // CURRENT panel state, not just whatever toggleStatusPanelForFocused()
    // last explicitly saved: a panel dragged wider/narrower without
    // ever being closed again before exit still needs its own latest
    // width persisted here, not just its open/closed flag.
    settings_.panelLayouts.clear();
    for (TorrentListWindow* w : windows) {
        AppSettings::PanelLayout layout;
        layout.statusOpen = w->isStatusPanelOpen();
        layout.statusWidth = w->statusPanelWidth();
        layout.filesOpen = w->isFilesPanelOpen();
        layout.filesWidth = w->filesPanelWidth();
        settings_.panelLayouts[w->serverName()] = layout;
    }

    // Whichever window has focus right now is the one brought back to
    // the front on the next launch (see the constructor).
    if (TorrentListWindow* focused = focusedListWindow()) {
        settings_.focusedServerAtClose = focused->serverName();
    }
    if (!windows.empty()) saveSettings(settings_);

    // Same backstop for the tracker/peers window, if one happens to
    // still be open — direct mouse/keyboard resizing or reordering on
    // its own header (not through "Manage columns...", which already
    // saves immediately on close) wouldn't otherwise be captured.
    // Several tracker windows could be open at once; picks whichever
    // one is found first, since they're meant to share a single layout
    // anyway (see AppSettings::trackerColumnWidths's own doc comment)
    // rather than needing to reconcile them against each other here.
    // Only captures whichever TAB that one window happens to be showing
    // right now, same scope "Manage columns..." itself has — the other
    // tab, if it was ever customized this session without switching
    // back to it before exit, isn't captured here either; a narrower
    // gap than it sounds, since TrackerPeerWindow's own switchToTab()
    // already keeps both tabs' layouts in sync with each other WITHIN
    // one running session, this just doesn't reach across a restart for
    // whichever one isn't currently showing.
    if (TDeskTop* deskTop = TProgram::deskTop) {
        if (deskTop->last) {
            TView* p = deskTop->last;
            do {
                p = p->next;
                if (auto* trackerWin = dynamic_cast<TrackerPeerWindow*>(p)) {
                    if (trackerWin->isPeersTabActive()) {
                        settings_.peerColumnWidths = trackerWin->columnWidths();
                        settings_.peerColumnOrder = trackerWin->columnOrder();
                        settings_.peerColumnVisible = trackerWin->columnVisibility();
                    } else {
                        settings_.trackerColumnWidths = trackerWin->columnWidths();
                        settings_.trackerColumnOrder = trackerWin->columnOrder();
                        settings_.trackerColumnVisible = trackerWin->columnVisibility();
                    }
                    saveSettings(settings_);
                    break;
                }
            } while (p != deskTop->last);
        }
    }
    TApplication::shutDown();
}

void App::idle() {
    TApplication::idle();
    // Refresh on a real interval (settings_.refreshIntervalSeconds),
    // no longer on every single event-loop tick — applied to EVERY open
    // torrent-list window on the same shared timer (there's one
    // refreshIntervalSeconds, not one per server), each independently
    // skipped while its own selection mode is active (see
    // TGridView::enterSelectionMode()): refresh() re-fetches and
    // re-applies the current sort/filter, which can reorder visible_ —
    // and TorrentListWindow::targetTorrents() maps a checked row
    // straight to visible_[row], so a reorder while rows are checked in
    // THAT window would silently apply an action to the wrong torrent.
    // Holding that one window still until its own selection finishes
    // (or is cancelled) avoids that outright, without needing to pause
    // every other window's own refresh too.
    //
    // Started here (non-blocking — see TransmissionClient::startRefresh()'s
    // own comment on why this matters with more than one server open),
    // not awaited: a window already mid-refresh from a PREVIOUS interval
    // (isAsyncRefreshInFlight() — a slow or unreachable server, bounded
    // by call()'s own 15s ceiling either way, but that can still span
    // more than one refreshIntervalSeconds) is left alone rather than
    // starting a second request on the same handle.
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRefresh_).count();
    if (elapsed >= settings_.refreshIntervalSeconds) {
        for (TorrentListWindow* w : allListWindows()) {
            if (!w->grid()->isInSelectionMode() && !w->isAsyncRefreshInFlight()) {
                w->startAsyncRefresh(multiHandle_);
            }
        }
        lastRefresh_ = now;
    }

    // Drives every in-flight request (however many windows started one
    // above, this interval or an earlier one still running) and applies
    // whichever ones have finished — every idle() tick, not just when
    // the timer above fires, so a finished request doesn't sit
    // unnoticed for up to a whole refreshIntervalSeconds before its own
    // window actually shows the result.
    if (multiHandle_) {
        int stillRunning = 0;
        curl_multi_perform(multiHandle_, &stillRunning);
        int msgsLeft = 0;
        CURLMsg* msg = nullptr;
        while ((msg = curl_multi_info_read(multiHandle_, &msgsLeft)) != nullptr) {
            if (msg->msg == CURLMSG_DONE) {
                // CURLOPT_PRIVATE was set to this window's own `this`
                // pointer when the request was started (see
                // TorrentListWindow::startAsyncRefresh()) — read back
                // here to know which window's own finishAsyncRefresh()
                // to call, without needing a separate lookup of
                // "which client does this easy handle belong to" of
                // this class's own.
                void* privateData = nullptr;
                curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &privateData);
                if (auto* w = static_cast<TorrentListWindow*>(privateData)) {
                    w->finishAsyncRefresh(multiHandle_);
                }
            }
        }
    }

    // Cheap (no RPC call, just reads data already cached by whichever
    // window has focus), so refreshed on every idle tick rather than
    // only alongside the interval-gated re-fetch above.
    updateBandwidthStatus();
    // "Manage columns..." is a single menu entry that acts on whichever
    // window currently has focus (see focusedGrid()) — checked here,
    // on every idle tick, rather than only when the menu is actually
    // opened, so the item is already greyed out (not just a no-op once
    // clicked) the moment focus moves to a window with nothing for it
    // to act on.
    if (focusedGrid()) enableCommand(cmManageColumns);
    else disableCommand(cmManageColumns);

    // The "Connections" menu's own bullet/highlight marks whichever
    // server currently has focus (see rebuildConnectionsMenu()) — kept
    // in sync here, on every idle tick, precisely because focus can
    // change in so many DIFFERENT ways (clicking a different window
    // directly, Window → Next, the Window List dialog, or this very
    // menu): checking once per tick, rather than threading a callback
    // through every single one of those paths individually, catches
    // all of them the same way updateBandwidthStatus() and
    // cmManageColumns' own enable/disable above already do for their
    // own "whatever has focus right now" state. A plain string compare
    // against the last-seen name is enough to tell whether it's
    // actually worth rebuilding — most ticks, nothing changed.
    std::string currentFocusedServer;
    if (TorrentListWindow* focused = focusedListWindow()) currentFocusedServer = focused->serverName();
    if (currentFocusedServer != lastConnectionsFocusedServer_) {
        lastConnectionsFocusedServer_ = currentFocusedServer;
        rebuildConnectionsMenu();
        rebuildPanelsMenu();
    }
}
