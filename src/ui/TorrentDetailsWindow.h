#pragma once

#define Uses_TWindow
#include <tvision/tv.h>
#include "../rpc/Torrent.h"

// Creates a window with the main information about a torrent (a
// snapshot taken when opened, it doesn't refresh itself).
TWindow* createTorrentDetailsWindow(const Torrent& t);
