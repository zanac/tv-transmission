#include "FilesSyncHelper.h"

#include "FilesPanel.h"
#include "TorrentFilesWindow.h"
#include "TorrentListWindow.h"

#define Uses_TDeskTop
#define Uses_TProgram
#define Uses_TView
#include <tvision/tv.h>

void refreshFilesPanelForTorrent(int torrentId) {
    TDeskTop* deskTop = TProgram::deskTop;
    if (!deskTop || !deskTop->last) return;
    // Same circular-list walk showFilesForSelected() already uses to
    // find an already-open TorrentFilesWindow for a given torrent id —
    // mirrored here, over TorrentListWindow instead, since a
    // FilesPanel is never a direct desktop child itself (it's embedded
    // inside whichever TorrentListWindow owns it).
    TView* p = deskTop->last;
    do {
        p = p->next;
        if (auto* win = dynamic_cast<TorrentListWindow*>(p)) {
            FilesPanel* panel = win->filesPanel();
            if (panel && panel->torrentId() == torrentId) panel->refresh();
        }
    } while (p != deskTop->last);
}

void refreshTorrentFilesWindowForTorrent(int torrentId) {
    TDeskTop* deskTop = TProgram::deskTop;
    if (!deskTop || !deskTop->last) return;
    TView* p = deskTop->last;
    do {
        p = p->next;
        if (auto* win = dynamic_cast<TorrentFilesWindow*>(p)) {
            if (win->torrentId() == torrentId) win->refresh();
        }
    } while (p != deskTop->last);
}
