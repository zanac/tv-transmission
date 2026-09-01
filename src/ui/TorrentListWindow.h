#pragma once

#define Uses_TWindow
#define Uses_TListViewer
#define Uses_TScrollBar
#define Uses_TEvent
#include <tvision/tv.h>

#include <vector>
#include "../rpc/TransmissionClient.h"
#include "../rpc/Torrent.h"

// Column used to sort the list; the numeric value matches the column
// order in each row (see kNameW/kDoneW/etc. and buildHeaderText() in
// TorrentListWindow.cpp) — also used to work out which header column
// was clicked.
enum class SortColumn { Name = 0, Done = 1, Size = 2, Down = 3, Up = 4, Added = 5, Status = 6 };

// TListViewer that renders one row per torrent: name, %, down/up rate.
class TorrentListViewer : public TListViewer {
public:
    TorrentListViewer(const TRect& r, TScrollBar* vScrollBar);

    void setTorrents(std::vector<Torrent> torrents);
    const Torrent* selectedTorrent() const;

    double totalDownloadRate() const; // sum of rateDownload across all torrents
    double totalUploadRate() const;   // sum of rateUpload across all torrents

    // Header column click: if it's already the active sort column, flips
    // direction; otherwise sorts by the new column (ascending). The
    // criterion is re-applied automatically on every refresh (see
    // setTorrents()), so it stays in effect over time.
    void toggleSort(SortColumn column);
    SortColumn sortColumn() const { return sortColumn_; }
    bool sortAscending() const { return sortAscending_; }

    void getText(char* dest, short item, short maxLen) override;

    // Fixed colors (white on blue; current row black on white),
    // independent of the app's overall theme. mapColor() is virtual on
    // TView, unlike getColor()/getPalette() which alone wouldn't be
    // enough here (TListViewer::draw() calls getColor(), which is not
    // virtual, but that in turn calls mapColor() on the real `this` —
    // so this override is still reached correctly).
    TColorAttr mapColor(uchar index) override;

private:
    void applySort(); // re-sorts torrents_ according to sortColumn_/sortAscending_

    std::vector<Torrent> torrents_;
    SortColumn sortColumn_ = SortColumn::Name;
    bool sortAscending_ = true;
};

class TorrentListWindow : public TWindow {
public:
    TorrentListWindow(const TRect& bounds, TransmissionClient& client);

    void handleEvent(TEvent& event) override; // catches the double-click

    void refresh();       // calls listTorrents() and updates the view
    void startSelected();
    void stopSelected();
    void removeSelected();
    void retranslate();   // re-applies the title in the current language

    double totalDownloadRate() const;
    double totalUploadRate() const;

private:
    void openDetailsForSelected();

    TransmissionClient& client_;
    TorrentListViewer* listViewer_ = nullptr;
};
