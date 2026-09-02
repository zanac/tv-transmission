#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#define Uses_TCheckBoxes
#include <tvision/tv.h>
#include <string>
#include "../rpc/Torrent.h"
#include "../rpc/TransmissionClient.h"

// TDialog rather than TWindow — not a cosmetic choice: TWindow and
// TDialog each have their OWN default getPalette() (cpBlueWindow/
// cpCyanWindow/cpGrayWindow vs. cpGrayDialog/cpBlueDialog/cpCyanDialog,
// see twindow.cpp/tdialog.cpp), and these assign different final colors
// to the same slots (e.g. buttons come out red here under TWindow's
// palette vs. green under TDialog's, for the exact same TButton code).
// Since the Settings dialog (a TDialog) is the look the user wants
// matched, this window needs to actually BE a TDialog, not just avoid
// overriding colors — a same-looking TWindow would still diverge because
// the two classes disagree on what a given palette index resolves to.
// It runs non-modally (inserted into the desktop like any other window,
// never execView()'d), so TDialog's Esc/Enter shortcuts are harmless
// here: TDialog::handleEvent only acts on them while `state & sfModal`,
// which is never set outside of execView().
//
// Also knows which torrent it's showing (so an already-open details
// window can be reused instead of duplicated) and handles its own
// "Apply"/"Close" buttons.
//
// Unlike the rest of this window's content (a snapshot taken when
// opened), the speed limit controls are live: "Apply" sends a
// torrent-set RPC call right away using torrentId_/client_.
class TorrentDetailsWindow : public TDialog {
public:
    TorrentDetailsWindow(const TRect& bounds, TStringView title,
                          int torrentId, const std::string& torrentName,
                          TransmissionClient& client);

    void handleEvent(TEvent& event) override;

    // So the caller (TorrentListWindow) can find an already-open details
    // window for a given torrent instead of opening a duplicate.
    int torrentId() const { return torrentId_; }

    // Set by createTorrentDetailsWindow() once the controls are built.
    TCheckBoxes* limitCheckboxes = nullptr;
    TInputLine* downloadLimitField = nullptr;
    TInputLine* uploadLimitField = nullptr;

private:
    void applySpeedLimits();
    void showTrackers();

    int torrentId_;
    std::string torrentName_;
    TransmissionClient& client_;
};

// Creates a window with the main information about a torrent (a
// snapshot taken when opened, it doesn't refresh itself) plus controls
// to set or clear a per-torrent download/upload speed limit override
// (applied immediately via `client` when confirmed).
TWindow* createTorrentDetailsWindow(const Torrent& t, TransmissionClient& client);
