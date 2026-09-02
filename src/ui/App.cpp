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
#define Uses_TFileDialog
#define Uses_MsgBox
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
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
            *new TMenuItem(tr(Str::MenuDeleteWithData), cmDeleteTorrentWithData, kbNoKey) +
            newLine() +
            *new TMenuItem(tr(Str::MenuStartNow), cmStartNowTorrent, kbNoKey) +
            *new TMenuItem(tr(Str::MenuVerify), cmVerifyTorrent, kbNoKey) +
            *new TMenuItem(tr(Str::MenuReannounce), cmReannounceTorrent, kbNoKey) +
            *new TMenuItem(tr(Str::MenuShowDetails), cmShowDetails, kbNoKey) +
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
    listWindow_ = new TorrentListWindow(r, client_,
        settings_.sortColumn, settings_.sortAscending,
        [this](SortColumn col, bool asc) {
            settings_.sortColumn = col;
            settings_.sortAscending = asc;
            saveSettings(settings_);
        });
    deskTop->insert(listWindow_);
}

void App::showAddTorrentDialog(const std::string& initialValue) {
    TInputLine* urlField = nullptr;
    auto* dlg = createAddTorrentDialog(urlField, initialValue);
    if (!dlg) return;
    ushort result = execView(dlg);
    std::string url = (result == cmOK) ? addTorrentDialogResult(urlField) : "";
    destroy(dlg);

    if (result == cmOK) {
        if (!url.empty()) {
            auto addResult = client_.addTorrent(url);
            if (addResult == TransmissionClient::AddTorrentResult::Duplicate)
                messageBox(tr(Str::MsgTorrentDuplicate), mfInformation | mfOKButton);
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

void App::showSettingsDialog() {
    // Live RPC call: the global speed limits aren't part of settings_ /
    // settings.json, they live on the Transmission daemon itself (see
    // TransmissionClient::getSessionLimits()). This uses whichever
    // connection settings are active *before* this dialog changes them —
    // if that connection doesn't work (e.g. this is the first time
    // host/user/password are being set up), the fetch fails and
    // `sessionLimitsFetched` says so.
    bool sessionLimitsFetched = false;
    SessionLimits sessionLimits = client_.getSessionLimits(&sessionLimitsFetched);

    SettingsDialogFields fields;
    if (auto* dlg = createSettingsDialog(settings_, sessionLimits, fields)) {
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

            // Only pushed back if the fetch above actually succeeded.
            // Otherwise the dialog's speed-limit fields were showing
            // meaningless defaults (0/disabled) rather than this
            // server's real state — most commonly on the very first
            // time host/user/password are configured, when the *old*
            // connection couldn't reach anything yet. Sending those
            // defaults to the newly-configured connection (now
            // reachable, thanks to applySettings() above) would
            // silently wipe out real limits already set there, even
            // though the user never touched the speed-limit fields.
            if (sessionLimitsFetched) {
                client_.setSessionLimits(settingsDialogSessionLimits(fields));
            }
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
        case cmDeleteTorrentWithData:
            if (listWindow_) listWindow_->deleteWithDataSelected();
            clearEvent(event);
            break;
        case cmStartNowTorrent:
            if (listWindow_) listWindow_->startNowSelected();
            clearEvent(event);
            break;
        case cmVerifyTorrent:
            if (listWindow_) listWindow_->verifySelected();
            clearEvent(event);
            break;
        case cmReannounceTorrent:
            if (listWindow_) listWindow_->reannounceSelected();
            clearEvent(event);
            break;
        case cmShowDetails:
            if (listWindow_) listWindow_->showDetailsForSelected();
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

