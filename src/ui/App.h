#pragma once

#define Uses_TApplication
#define Uses_TMenuBar
#define Uses_TStatusLine
#include <tvision/tv.h>

#include <chrono>
#include "../AppSettings.h"
#include "../rpc/TransmissionClient.h"

class TorrentListWindow;

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

private:
    void newTorrentListWindow();
    void showAddTorrentDialog(const std::string& initialValue = "");
    void showSettingsDialog();
    void showFilterDialog();
    void showWindowListDialog();
    void showAboutDialog();
    void applySettings(); // reconfigures client_ after a settings change
    void updateBandwidthStatus(); // updates the D:/U: text in the status bar

    AppSettings settings_;
    TransmissionClient client_;
    TorrentListWindow* listWindow_ = nullptr;
    std::chrono::steady_clock::time_point lastRefresh_;
};

// Custom application commands (> tvision's cmUserBase)
const ushort cmAddTorrent       = 100;
const ushort cmStartTorrent     = 101;
const ushort cmStopTorrent      = 102;
const ushort cmRemoveTorrent    = 103;
const ushort cmSettings         = 104;
const ushort cmBandwidthDisplay = 105; // non-clickable item in the status bar
const ushort cmShowWindowList   = 106;
const ushort cmVerifyTorrent    = 107;
const ushort cmReannounceTorrent= 108;
const ushort cmStartNowTorrent  = 109;
const ushort cmShowDetails      = 110;
const ushort cmDeleteTorrentWithData = 111;
const ushort cmAbout            = 112;
const ushort cmFilters          = 113;
