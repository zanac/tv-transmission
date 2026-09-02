#pragma once

#define Uses_TWindow
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TEvent
#include <tvision/tv.h>

#include <functional>
#include <vector>
#include "../AppSettings.h"
#include "../rpc/TransmissionClient.h"
#include "../rpc/Torrent.h"

// Called whenever the user changes the sort column/direction (header
// click), so the caller can persist it (see App::newTorrentListWindow()).
using SortChangedCallback = std::function<void(SortColumn, bool)>;

// TListViewer that renders one row per torrent: name, %, down/up rate.
class TorrentListViewer : public TListViewer {
public:
    TorrentListViewer(const TRect& r, TScrollBar* vScrollBar,
                       SortColumn initialSort, bool initialAscending,
                       SortChangedCallback onSortChanged);

    void setTorrents(std::vector<Torrent> torrents);
    const Torrent* selectedTorrent() const;

    double totalDownloadRate() const; // sum of rateDownload across all torrents
    double totalUploadRate() const;   // sum of rateUpload across all torrents

    // Header column click: if it's already the active sort column, flips
    // direction; otherwise sorts by the new column (ascending). The
    // criterion is re-applied automatically on every refresh (see
    // setTorrents()), so it stays in effect over time. Also invokes
    // onSortChanged so it can be persisted.
    void toggleSort(SortColumn column);
    SortColumn sortColumn() const { return sortColumn_; }
    bool sortAscending() const { return sortAscending_; }

    void getText(char* dest, short item, short maxLen) override;

    // Called on construction, after every refresh, and whenever the
    // focused row changes (see focusItem() below) — enables/disables
    // App.h's torrent commands (cmStartTorrent, cmStopTorrent, etc.)
    // according to the now-selected torrent's state. This uses
    // tvision's own global enable/disable mechanism (TView::
    // enableCommand()/disableCommand(), backed by a single shared
    // TCommandSet — see mapcolor... no, see tview.cpp's
    // curCommandSet), so it automatically grays out matching items in
    // the Torrent menu, the status bar, AND the right-click context
    // menu below all at once, without having to touch any of them
    // individually.
    void updateCommandStates();
    void focusItem(short item) override; // calls updateCommandStates()

    // Fixed colors (white on blue; current row black on white),
    // independent of the app's overall theme. mapColor() is virtual on
    // TView, unlike getColor()/getPalette() which alone wouldn't be
    // enough here (TListViewer::draw() calls getColor(), which is not
    // virtual, but that in turn calls mapColor() on the real `this` —
    // so this override is still reached correctly).
    TColorAttr mapColor(uchar index) override;

    void handleEvent(TEvent& event) override; // right-click context menu

private:
    void applySort(); // re-sorts torrents_ according to sortColumn_/sortAscending_
    void showContextMenu(TPoint where);

    std::vector<Torrent> torrents_;
    SortColumn sortColumn_;
    bool sortAscending_;
    SortChangedCallback onSortChanged_;
};

class TorrentListWindow : public TWindow {
public:
    TorrentListWindow(const TRect& bounds, TransmissionClient& client,
                       SortColumn initialSort, bool initialAscending,
                       SortChangedCallback onSortChanged);

    void handleEvent(TEvent& event) override; // catches the double-click

    void refresh();       // calls listTorrents() and updates the view
    void startSelected();
    void stopSelected();
    void removeSelected();       // confirmation prompt, then keeps files on disk
    void deleteWithDataSelected(); // confirmation prompt, then deletes files too
    void startNowSelected();
    void verifySelected();
    void reannounceSelected();
    void showDetailsForSelected();
    void retranslate();   // re-applies the title in the current language

    double totalDownloadRate() const;
    double totalUploadRate() const;

private:
    TransmissionClient& client_;
    TorrentListViewer* listViewer_ = nullptr;
};
