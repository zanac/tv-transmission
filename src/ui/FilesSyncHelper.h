#pragma once

// Keeps FilesPanel (the side panel embedded in a TorrentListWindow) and
// TorrentFilesWindow (the separate, full-size files window) in sync
// with each other when both happen to be open for the same torrent at
// once — reported directly: toggling a file's own wanted state in one
// left the other showing stale data until something else (switching
// away and back, for FilesPanel; nothing at all, for TorrentFilesWindow,
// which has no periodic refresh of its own) happened to reload it.
//
// Neither view keeps a registry of the other's own instances — both
// just walk TProgram::deskTop's own child list (the same "find an
// already-open window for this torrent id" pattern
// TorrentListWindow::showFilesForSelected() already uses to avoid
// opening a duplicate), so there's nothing to keep registered or torn
// down: a closed view simply isn't found the next time either function
// runs, the same as it isn't found by that existing pattern either.

void refreshFilesPanelForTorrent(int torrentId);
void refreshTorrentFilesWindowForTorrent(int torrentId);
