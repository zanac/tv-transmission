#pragma once

#define Uses_TGroup
#define Uses_TEvent
#include <tvision/tv.h>

#include "../rpc/Torrent.h"
#include "../rpc/TransmissionClient.h"
#include "../tvision-ext/TGridView.h"
#include "TorrentFilesWindow.h" // FileTreeRow, buildFileTreeRows() — reused, not duplicated

// The "Files" side panel (Window -> Panels -> Files) — a reduced
// companion to TorrentFilesWindow (name + wanted only, none of that
// window's own size/progress/priority columns or right-click menu),
// live-updating to whichever torrent was last single-clicked in the
// main list while this panel is open (see TorrentListWindow's own
// row-click wiring). Reuses buildFileTreeRows() (TorrentFilesWindow.h)
// for the same folder-grouped display that window itself uses, rather
// than a second, separately-maintained flat-vs-tree implementation.
//
// 0x30 (the same cyan the Status panel's own checkboxes use) throughout
// — rows, header, and the vertical scrollbar alike, asked for directly
// over two earlier looks (a precisely-computed cyan/green "ListViewer"
// dialog scheme, then a plain gray/black). TGridView's own row/header
// colors are fixed defaults, not palette-resolved (see its own
// README), so reaching them needs this panel's own explicit
// setRowColorCallback()/setHeaderColorCallback() (see the .cpp); the
// scrollbar, by contrast, IS a plain TScrollBar resolving through the
// ordinary palette chain, reached instead through this panel's own
// mapColor() override (see the .cpp for why one fallback covers it
// without needing StatusPanel's own multi-case table). Only the
// background around the grid (the separate TPanelBackground child, see
// the .cpp) needs its own explicit fill, since TGroup itself paints
// nothing of its own.
//
// Never independently movable or resizable by the user, same as
// StatusPanel — TorrentListWindow is the only thing that ever calls
// changeBounds() on it.
class FilesPanel : public TGroup {
public:
    FilesPanel(const TRect& bounds, TransmissionClient& client);

    void handleEvent(TEvent& event) override;
    TColorAttr mapColor(uchar) override;

    // Switches to showing a different torrent's own files — safe to
    // call with the same torrentId already showing (just re-fetches),
    // which is exactly what happens on every periodic refresh tick
    // while a torrent is already selected (see TorrentListWindow's own
    // refresh cycle).
    void showTorrent(int torrentId, const std::string& torrentName);
    int torrentId() const { return torrentId_; }

private:
    void refresh();
    void toggleWantedForFocused();

    TransmissionClient& client_;
    TGridView* grid_ = nullptr;
    int torrentId_ = -1;
    std::vector<TorrentFile> files_;
    std::vector<FileTreeRow> rows_;
};
