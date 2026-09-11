#include "App.h"
#include "TorrentListWindow.h"
#include "TorrentDetailsWindow.h"
#include "TorrentFilesWindow.h"
#include "TrackerListWindow.h"
#include "AddTorrentDialog.h"
#include "ConnectionDialog.h"
#include "ServerSettingsDialog.h"
#include "FilterDialog.h"
#include "../tgridview/TGridColumnManagerDialog.h"
#include "WindowListDialog.h"
#include "AboutDialog.h"
#include "BandwidthStatusLine.h"
#include "../tvision-ext/TComboBox.h"
#include "Strings.h"
#include "../Config.h"

#define Uses_TDeskTop
#define Uses_TGroup
#define Uses_TWindow
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TKeys
#define Uses_TEvent
#define Uses_TFileDialog
#define Uses_MsgBox
#include <tvision/tv.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

// Mirrors ConnectionDialog.cpp's own buildServerItems() (same idea:
// one TComboItem per configured server, alphabetical since
// AppSettings::servers is a std::map) — kept as a separate, local copy
// rather than shared, matching how this project already keeps each
// file's own small helpers local (see e.g. TorrentListWindow.cpp's
// buildWindowTitle()) rather than growing a shared-utilities file for
// a couple of lines of logic. The one difference: focusedName here is
// whichever server actually ends up on top at startup (App::App() — it
// may differ from ConnectionDialog's own notion of "the" active
// server), not settings.activeServer.
TComboItem* buildServerComboItems(const AppSettings& settings, const std::string& focusedName,
                                   short& focusedIndex) {
    TComboItem* head = nullptr;
    TComboItem* tail = nullptr;
    focusedIndex = 0;
    short idx = 0;
    for (const auto& [name, profile] : settings.servers) {
        (void)profile;
        TComboItem* item = new TComboItem(name.c_str(), 0, nullptr);
        if (name == focusedName) focusedIndex = idx;
        if (head == nullptr) head = tail = item;
        else { tail->next = item; tail = item; }
        idx++;
    }
    return head;
}

} // namespace


App::App(const AppSettings& initialSettings)
    : TProgInit(&App::initStatusLine, &App::initMenuBar, &TApplication::initDeskTop),
      settings_(initialSettings) {
    // The global language was already set by main() BEFORE constructing
    // this object (see the comment in App.h): initMenuBar()/
    // initStatusLine() have therefore already read it correctly. No need
    // to redo it here.
    lastRefresh_ = std::chrono::steady_clock::now();

    // Reserves the row directly below the menu bar for the server combo
    // box built further down — every server window below fills
    // deskTop's own extent (see TorrentListWindow's own fullScreen=true
    // constructor call), so this has to happen BEFORE any of them are
    // created, or they'd be sized against the old, taller extent and
    // then immediately overlap the combo box until the next terminal
    // resize came along to correct it via growMode. deskTop has no
    // children yet at this point in the constructor, so there's nothing
    // for this relayout to disturb.
    TRect deskRect = deskTop->getExtent();
    TRect comboRowRect = deskRect;
    comboRowRect.b.y = comboRowRect.a.y + 1;
    deskRect.a.y += 1;
    // changeBounds() rather than locate(): this is the same primitive
    // tvision itself calls on deskTop when the terminal is resized
    // (cascading down from TProgram, via deskTop's own growMode) — the
    // direct "resize this view and relayout its children accordingly"
    // operation, with none of locate()'s extra size-limit-clamping/
    // centering logic that's meant for interactive dialogs, not a
    // programmatic one-time desktop resize like this.
    deskTop->changeBounds(deskRect);

    // Every configured server gets its own window on startup — see
    // AppSettings::servers' own comment on why there's no longer just
    // one "active" connection. Whichever one had focus when the app was
    // last closed (see shutDown()) ends up on top again; if that name
    // no longer matches anything (removed since, or a first run),
    // whichever opens first just stays wherever TWindow::insert() put
    // it. Always opened at the full (now combo-row-adjusted) desktop
    // extent — settings_.windowLayouts, still written by shutDown() for
    // whatever future use, is no longer read back here: every server
    // window is fullScreen now (see TorrentListWindow's own
    // constructor), so a previous session's saved size/position — quite
    // possibly smaller, from before this app enforced full-desktop
    // windows — would otherwise flash on screen for a frame before the
    // next terminal-resize event's growMode correction fixed it.
    TorrentListWindow* toFocus = nullptr;
    for (const auto& [name, profile] : settings_.servers) {
        (void)profile; // only the name is needed here — openServerWindow() looks up the profile itself
        TorrentListWindow* win = openServerWindow(name, deskTop->getExtent());
        if (win && name == settings_.focusedServerAtClose) toFocus = win;
    }
    if (toFocus) toFocus->select();

    // The server combo box itself, in the row just reserved above —
    // picking a name from it brings that server's window to the front
    // (see handleEvent()'s own cmComboBoxSelectionChanged case), which
    // is the only way to do that by name now that server windows are
    // fullScreen and simply stack on top of each other (Ctrl+F6 "Next"
    // still cycles through them too, just not by name). Built AFTER the
    // loop above, not before, so its own initially-focused entry can
    // match whichever window really did end up on top (toFocus), rather
    // than guessing independently and risking the two disagreeing right
    // from startup.
    short focusedIdx = 0;
    TComboItem* serverItems = buildServerComboItems(settings_,
        toFocus ? toFocus->serverName() : std::string(), focusedIdx);
    TRect comboBounds(comboRowRect.a.x + 1, comboRowRect.a.y,
                       std::min(comboRowRect.a.x + 1 + 32, comboRowRect.b.x - 1), comboRowRect.b.y);
    serverCombo_ = new TComboBox(comboBounds, serverItems, focusedIdx);
    // Deliberately growMode = 0 (the TView default, left unset): this
    // stays pinned to its fixed width and position in the top-left
    // corner, directly under the menu bar, regardless of terminal
    // resizes — unlike the server windows themselves (see
    // TGridWindow.cpp), which are MEANT to track the terminal size.
    insert(serverCombo_);
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
            newLine() +
            *new TMenuItem(tr(Str::MenuSelectMultiple), cmSelectMultiple, kbNoKey) +
            newLine() +
            *new TMenuItem(tr(Str::MenuQuit), cmQuit, kbAltX) +
        *new TSubMenu(tr(Str::MenuWindow), kbAltW) +
            // Standard tvision commands. Server windows are now
            // fullScreen (see TorrentListWindow's own constructor) and
            // therefore NOT tileable — Zoom/Tile/Cascade only ever act
            // on the "Torrent details"/files/tracker windows, which
            // still are. Next (Ctrl+F6) still cycles through every
            // window regardless, server ones included — same as the
            // new server combo box below the menu bar, just keyboard-
            // driven instead of picked by name.
            *new TMenuItem(tr(Str::MenuWindowZoom), cmZoom, kbCtrlF5) +
            *new TMenuItem(tr(Str::MenuWindowNext), cmNext, kbCtrlF6) +
            *new TMenuItem(tr(Str::MenuWindowClose), cmClose, kbAltF3) +
            newLine() +
            *new TMenuItem(tr(Str::MenuWindowTile), cmTile, kbNoKey) +
            *new TMenuItem(tr(Str::MenuWindowCascade), cmCascade, kbNoKey) +
            newLine() +
            *new TMenuItem(tr(Str::MenuWindowList), cmShowWindowList, kbAlt0) +
        *new TSubMenu(tr(Str::MenuColumnsMenu), kbNoKey) +
            *new TMenuItem(tr(Str::MenuFilters), cmFilters, kbNoKey) +
            // Rationalized from what used to be three separate entry
            // points here (a "Resize columns" submenu, an "Order
            // columns" submenu, and a standalone "Columns..." dialog —
            // each nested with the same (TMenuItem&) cast idiom that
            // was needed for those two submenus, no longer needed now
            // that there's only one plain item) into the single column
            // manager dialog — see ColumnManagerDialog.h.
            *new TMenuItem(tr(Str::MenuManageColumns), cmManageColumns, kbNoKey) +
        *new TSubMenu(tr(Str::MenuSettingsMenu), kbNoKey) +
            *new TMenuItem(tr(Str::MenuConnection), cmSettings, kbF9) +
            *new TMenuItem(tr(Str::MenuServerSettings), cmServerSettings, kbNoKey) +
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

TorrentListWindow* App::openServerWindow(const std::string& name, const TRect& bounds) {
    // Already open: bring it forward instead of duplicating — this is
    // also how a server just added/edited in the Connection dialog
    // (see showConnectionDialog()) reaches an existing window rather
    // than always opening a new one.
    for (TorrentListWindow* w : allListWindows()) {
        if (w->serverName() == name) {
            w->select();
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
                                                         profile.user, profile.password);
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

    auto* win = new TorrentListWindow(bounds, name, clientRef,
        settings_.sortColumn, settings_.sortAscending, settings_.filter,
        columnLayout.widths, columnLayout.order, columnLayout.visible,
        [this](SortColumn col, bool asc) {
            settings_.sortColumn = col;
            settings_.sortAscending = asc;
            saveSettings(settings_);
        },
        settings_.trackerColumnWidths, settings_.trackerColumnOrder, settings_.trackerColumnVisible);
    deskTop->insert(win); // TorrentListWindow's own constructor already calls refresh() at the end — nothing more needed here
    return win;
}

void App::refreshServerCombo() {
    if (!serverCombo_) return;
    // Whatever name is currently shown stays focused if it's still in
    // the rebuilt list — buildServerComboItems() falls back to the
    // first entry (or an empty box, if settings_.servers is now empty
    // entirely) when it isn't, e.g. right after this same name was
    // just removed.
    std::string currentName = serverCombo_->editText();
    short focusedIdx = 0;
    TComboItem* items = buildServerComboItems(settings_, currentName, focusedIdx);
    serverCombo_->newList(items, focusedIdx);
}

void App::syncServerCombo() {
    if (!serverCombo_) return;
    TorrentListWindow* focused = focusedListWindow();
    if (!focused) return; // some other window (or the combo itself) has focus — nothing to sync FROM
    if (focused->serverName() == serverCombo_->editText()) return; // already in sync — see this method's own comment in App.h for why this check has to come first
    auto values = serverCombo_->allValues();
    auto it = std::find(values.begin(), values.end(), focused->serverName());
    if (it == values.end()) return; // defensive; every configured server always has a matching combo entry (see refreshServerCombo()), so this shouldn't actually happen
    serverCombo_->focusItem((short)std::distance(values.begin(), it));
}

void App::showAddTorrentDialog(const std::string& initialValue) {
    // Adds to whichever server's window currently has focus — the same
    // "act on the focused one" rule every other Torrent-menu command
    // follows now that there's more than one to choose from.
    TorrentListWindow* target = focusedListWindow();
    if (!target) return;
    auto clientIt = clients_.find(target->serverName());
    if (clientIt == clients_.end()) return;

    TInputLine* urlField = nullptr;
    auto* dlg = createAddTorrentDialog(urlField, initialValue);
    if (!dlg) return;
    ushort result = execView(dlg);
    std::string url = (result == cmOK) ? addTorrentDialogResult(urlField) : "";
    destroy(dlg);

    if (result == cmOK) {
        if (!url.empty()) {
            auto addResult = clientIt->second->addTorrent(url);
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
                    clientIt->second->lastError().c_str());
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
        // simpler than maintaining a hand-built browser.
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
        // confirms (or edits further, or cancels) from here.
        showAddTorrentDialog(chosenPath);
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
        refreshServerCombo();
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
                target->refresh();
            }
            target->select();
        } else {
            openServerWindow(name, deskTop->getExtent());
        }
        refreshServerCombo();
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
    if (auto* dlg = createServerSettingsDialog(sessionLimits, fields)) {
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

void App::showFilterDialog() {
    // Shared across every open window (see AppSettings::filter's own
    // comment — the same "one setting, every window" choice as column
    // widths/order), so confirming this applies it everywhere at once
    // rather than only to whichever window has focus.
    FilterDialogFields fields;
    if (auto* dlg = createFilterDialog(settings_.filter, fields)) {
        if (execView(dlg) == cmOK) {
            settings_.filter = filterDialogResult(fields);
            saveSettings(settings_); // persisted right away, same as everything else in Config.h
            for (TorrentListWindow* w : allListWindows()) {
                w->setFilter(settings_.filter); // applied to already-fetched data, no re-fetch
            }
        }
        destroy(dlg);
    }
}

void App::showColumnManagerDialog() {
    // Acts on whichever window currently has focus — see focusedGrid()
    // and idle() (which keeps the menu item itself disabled whenever
    // this would come back null, so reaching here with none is only a
    // defensive fallback against a focus change slipping in between the
    // command firing and this running).
    TGridView* grid = focusedGrid();
    if (!grid) return;

    // This dialog lives in tgridview/ (see its own README.md) and has
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
        } else if (auto* trackerWin = dynamic_cast<TrackerListWindow*>(TProgram::deskTop->current)) {
            settings_.trackerColumnWidths = trackerWin->columnWidths();
            settings_.trackerColumnOrder = trackerWin->columnOrder();
            settings_.trackerColumnVisible = trackerWin->columnVisibility();
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
    // TGridWindow base), TrackerListWindow, TorrentFilesWindow —
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
            } else if (auto* w = dynamic_cast<TrackerListWindow*>(p)) {
                if (w->clientPtr() == client) toClose.push_back(w);
            }
        } while (p != TProgram::deskTop->last);
    }
    for (TWindow* w : toClose) w->close();
}

void App::handleEvent(TEvent& event) {
    TApplication::handleEvent(event);

    if (event.what == evBroadcast && event.message.command == cmComboBoxSelectionChanged &&
        event.message.infoPtr == serverCombo_) {
        // Picking a server from the combo brings its window to the
        // front — openServerWindow() already does exactly that (its
        // "already open" branch, which every configured server always
        // hits — see App::App()) rather than opening a new one, so
        // there's nothing else to do here beyond calling it.
        openServerWindow(serverCombo_->editText(), deskTop->getExtent());
        clearEvent(event);
        return;
    }

    if (event.what != evCommand) return;

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
        case cmSettings:
            showConnectionDialog();
            clearEvent(event);
            break;
        case cmServerSettings:
            showServerSettingsDialog();
            clearEvent(event);
            break;
        case cmFilters:
            showFilterDialog();
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
    // place), same reasoning as windowLayouts just below: every
    // configured server always has its own open window (the MDI
    // invariant this app maintains — see its own constructor), so
    // rebuilding from allListWindows() can't lose a legitimate one, and
    // a server removed via the Connection dialog during this session
    // doesn't leave a stale entry behind either.
    std::vector<TorrentListWindow*> windows = allListWindows();
    settings_.columnLayouts.clear();
    for (TorrentListWindow* w : windows) {
        AppSettings::ColumnLayout layout;
        layout.widths = w->columnWidths();
        layout.order = w->columnOrder();
        layout.visible = w->columnVisibility();
        settings_.columnLayouts[w->serverName()] = layout;
    }

    // Every still-open window's own position/size, keyed by its server
    // name — rebuilt from scratch each time (not just updated in place)
    // so a server closed this session, or removed via the Connection
    // dialog, doesn't leave a stale entry behind from a previous run.
    settings_.windowLayouts.clear();
    for (TorrentListWindow* w : windows) {
        TRect r = w->getBounds();
        WindowLayout layout;
        layout.x = r.a.x;
        layout.y = r.a.y;
        layout.w = r.b.x - r.a.x;
        layout.h = r.b.y - r.a.y;
        settings_.windowLayouts[w->serverName()] = layout;
    }
    // Whichever window has focus right now is the one brought back to
    // the front on the next launch (see the constructor).
    if (TorrentListWindow* focused = focusedListWindow()) {
        settings_.focusedServerAtClose = focused->serverName();
    }
    if (!windows.empty()) saveSettings(settings_);

    // Same backstop for the tracker list, if one happens to still be
    // open — direct mouse/keyboard resizing or reordering on its own
    // header (not through "Manage columns...", which already saves
    // immediately on close) wouldn't otherwise be captured. Several
    // tracker windows could be open at once; picks whichever one is
    // found first, since they're meant to share a single layout anyway
    // (see AppSettings::trackerColumnWidths's own doc comment) rather
    // than needing to reconcile them against each other here.
    if (TDeskTop* deskTop = TProgram::deskTop) {
        if (deskTop->last) {
            TView* p = deskTop->last;
            do {
                p = p->next;
                if (auto* trackerWin = dynamic_cast<TrackerListWindow*>(p)) {
                    settings_.trackerColumnWidths = trackerWin->columnWidths();
                    settings_.trackerColumnOrder = trackerWin->columnOrder();
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
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRefresh_).count();
    if (elapsed >= settings_.refreshIntervalSeconds) {
        for (TorrentListWindow* w : allListWindows()) {
            if (!w->grid()->isInSelectionMode()) w->refresh();
        }
        lastRefresh_ = now;
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
    // Same reasoning as the two checks just above: cheap, no RPC
    // involved, so just done on every tick rather than hooked into
    // every individual way focus can change (Ctrl+F6, Alt+0's window
    // list, clicking a window directly, ...) separately.
    syncServerCombo();
}
