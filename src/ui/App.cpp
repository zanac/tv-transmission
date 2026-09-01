#include "App.h"
#include "TorrentListWindow.h"
#include "AddTorrentDialog.h"
#include "SettingsDialog.h"
#include "WindowListDialog.h"
#include "BandwidthStatusLine.h"
#include "Strings.h"
#include "../Config.h"

#define Uses_TDeskTop
#define Uses_TWindow
#define Uses_TSubMenu
#define Uses_TMenuItem
#define Uses_TStatusDef
#define Uses_TStatusItem
#define Uses_TKeys
#define Uses_TEvent
#include <tvision/tv.h>

#include <cstdio>
#include <vector>

App::App(const AppSettings& initialSettings)
    : TProgInit(&App::initStatusLine, &App::initMenuBar, &TApplication::initDeskTop),
      settings_(initialSettings),
      client_(settings_.host, settings_.port, settings_.user, settings_.password) {
    // The global language was already set by main() BEFORE constructing
    // this object (see the comment in App.h): initMenuBar()/
    // initStatusLine() have therefore already read it correctly. No need
    // to redo it here.
    lastRefresh_ = std::chrono::steady_clock::now();
    newTorrentListWindow();
}

TMenuBar* App::initMenuBar(TRect r) {
    r.b.y = r.a.y + 1;
    return new TMenuBar(r,
        *new TSubMenu(tr(Str::MenuTorrent), kbAltT) +
            *new TMenuItem(tr(Str::MenuAdd), cmAddTorrent, kbF2) +
            *new TMenuItem(tr(Str::MenuStart), cmStartTorrent, kbF5) +
            *new TMenuItem(tr(Str::MenuStop), cmStopTorrent, kbF6) +
            *new TMenuItem(tr(Str::MenuRemove), cmRemoveTorrent, kbF8) +
            newLine() +
            *new TMenuItem(tr(Str::MenuSettings), cmSettings, kbF9) +
            newLine() +
            *new TMenuItem(tr(Str::MenuQuit), cmQuit, kbAltX) +
        *new TSubMenu(tr(Str::MenuWindow), kbAltW) +
            // Standard tvision commands: the main window can't be closed
            // (see TorrentListWindow, wfClose removed), but the "Torrent
            // details" windows (double-click, non-modal) can stay open
            // more than one at a time — hence the need to be able to
            // tile/close/cycle through them.
            *new TMenuItem(tr(Str::MenuWindowZoom), cmZoom, kbCtrlF5) +
            *new TMenuItem(tr(Str::MenuWindowNext), cmNext, kbCtrlF6) +
            *new TMenuItem(tr(Str::MenuWindowClose), cmClose, kbAltF3) +
            newLine() +
            *new TMenuItem(tr(Str::MenuWindowTile), cmTile, kbNoKey) +
            *new TMenuItem(tr(Str::MenuWindowCascade), cmCascade, kbNoKey) +
            newLine() +
            *new TMenuItem(tr(Str::MenuWindowList), cmShowWindowList, kbAlt0)
    );
}

TStatusLine* App::initStatusLine(TRect r) {
    r.a.y = r.b.y - 1;
    return new BandwidthStatusLine(r,
        *new TStatusDef(0, 0xFFFF) +
            // Text updated at runtime by updateBandwidthStatus();
            // kbNoKey because it's not an action, just information.
            *new TStatusItem("D: --  U: --", kbNoKey, cmBandwidthDisplay) +
            *new TStatusItem(tr(Str::StatusAdd), kbF2, cmAddTorrent) +
            *new TStatusItem(tr(Str::StatusStart), kbF5, cmStartTorrent) +
            *new TStatusItem(tr(Str::StatusStop), kbF6, cmStopTorrent) +
            *new TStatusItem(tr(Str::StatusSettings), kbF9, cmSettings) +
            *new TStatusItem(tr(Str::StatusQuit), kbAltX, cmQuit)
    );
}

void App::newTorrentListWindow() {
    // Full desktop extent and "locked" (see flags = 0 in
    // TorrentListWindow's constructor): this is the main window, meant
    // to always stay maximized.
    TRect r = deskTop->getExtent();
    listWindow_ = new TorrentListWindow(r, client_);
    deskTop->insert(listWindow_);
}

void App::showAddTorrentDialog() {
    TInputLine* urlField = nullptr;
    if (auto* dlg = createAddTorrentDialog(urlField)) {
        if (execView(dlg) == cmOK) {
            std::string url = addTorrentDialogResult(urlField);
            if (!url.empty())
                client_.addTorrent(url);
        }
        destroy(dlg);
    }
}

void App::showSettingsDialog() {
    SettingsDialogFields fields;
    if (auto* dlg = createSettingsDialog(settings_, fields)) {
        if (execView(dlg) == cmOK) {
            settings_ = settingsDialogResult(fields, settings_);
            applySettings();
            saveSettings(settings_); // persisted right away: see Config.h
            setLanguage(settings_.language);
            // The main window already exists: relabel it right away.
            // The menu bar and status bar, on the other hand, are built
            // only once at startup (see main.cpp/App.h) and stay in
            // whatever language was active then until the app is
            // restarted — but from this point on restarting *works*:
            // the config file now holds the chosen language.
            if (listWindow_) listWindow_->retranslate();
        }
        destroy(dlg);
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

void App::applySettings() {
    client_.setEndpoint(settings_.host, settings_.port);
    client_.setCredentials(settings_.user, settings_.password);
    if (listWindow_) listWindow_->refresh();
}

void App::updateBandwidthStatus() {
    if (!statusLine || !listWindow_) return;
    double down = listWindow_->totalDownloadRate();
    double up = listWindow_->totalUploadRate();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "D: %.1f KB/s  U: %.1f KB/s",
                  down / 1024.0, up / 1024.0);
    static_cast<BandwidthStatusLine*>(statusLine)->setItemText(cmBandwidthDisplay, buf);
}

void App::handleEvent(TEvent& event) {
    TApplication::handleEvent(event);
    if (event.what != evCommand) return;

    switch (event.message.command) {
        case cmAddTorrent:
            showAddTorrentDialog();
            clearEvent(event);
            break;
        case cmStartTorrent:
            if (listWindow_) listWindow_->startSelected();
            clearEvent(event);
            break;
        case cmStopTorrent:
            if (listWindow_) listWindow_->stopSelected();
            clearEvent(event);
            break;
        case cmRemoveTorrent:
            if (listWindow_) listWindow_->removeSelected();
            clearEvent(event);
            break;
        case cmSettings:
            showSettingsDialog();
            clearEvent(event);
            break;
        case cmShowWindowList:
            showWindowListDialog();
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

void App::idle() {
    TApplication::idle();
    // Refresh on a real interval (settings_.refreshIntervalSeconds),
    // no longer on every single event-loop tick.
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastRefresh_).count();
    if (elapsed >= settings_.refreshIntervalSeconds) {
        if (listWindow_) listWindow_->refresh();
        lastRefresh_ = now;
    }
    // Total bandwidth is refreshed on every idle tick: it's cheap (no
    // RPC call, just reads data already cached by listWindow_).
    updateBandwidthStatus();
}

