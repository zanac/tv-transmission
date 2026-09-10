#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#include <tvision/tv.h>
#include <functional>
#include "../AppSettings.h"
#include "LanguageComboBox.h"

// Direct pointers to the Connection dialog's input fields.
//
// IMPORTANT: these fields used to be "found" after the dialog was
// created by scanning the TGroup's child list in the order encountered
// starting from `last`. That order does NOT match insertion order:
// TGroup::insert() inserts every new view at the head of the circular
// list, so scanning it gives the REVERSE of insertion order (verified
// empirically). With 5 fields, the result was that host/user and
// refresh/password ended up swapped with each other (the port, being
// the middle field, happened to look correct by pure coincidence). To
// eliminate this whole class of bug, the dialog returns pointers to the
// fields here at creation time, instead of having to "guess" them
// afterwards.
struct ConnectionDialogFields {
    TInputLine* refreshInterval = nullptr;
    // Editable combo of logical server names (see AppSettings::servers)
    // — "[+]" adds a name to the list itself, without saving anything
    // under it yet: that only happens once Save/OK (see its own button
    // comment in ConnectionDialog.cpp) successfully tests the
    // connection currently shown for whichever name this is. "[-]" is
    // NOT symmetric with either of those: removing a server takes
    // effect immediately, before Save/OK is ever pressed — see
    // ServerRemovedCallback below for why.
    TComboBox* serverName = nullptr;
    TInputLine* host = nullptr;
    TInputLine* port = nullptr;
    TInputLine* user = nullptr;
    TInputLine* password = nullptr;
    LanguageComboBox* language = nullptr;
};

// Called once a server's connection details have actually been tested
// successfully and should be persisted — from the Save/OK button (see
// its own doc comment below) — with the logical name currently shown
// and the profile to save it under. Same immediacy as
// ServerRemovedCallback above and for the same reason: the caller
// (App::showConnectionDialog()) is expected to save it to settings_,
// persist that to disk, and open (or update) that server's own
// torrent-list window right here — not wait for this dialog to close,
// since Save deliberately does NOT close it (see the button's own
// comment).
using ServerSavedCallback = std::function<void(const std::string& name, const ServerProfile& profile)>;

// Called the instant "[-]" actually removes a server from the combo
// (not for the no-op case — see TComboBox::removeCurrentValue()'s own
// comment — where the current text doesn't match any entry), with that
// server's own logical name. Deliberately immediate rather than folded
// into connectionDialogResult() the way everything else in this dialog
// is: a server the user just removed keeping its own torrent-list
// window (and any Details/Files/Tracker window still open for one of
// its torrents) open until OK is *also* pressed — or never, if Cancel
// is pressed instead, quietly undoing a removal that already showed
// its own "Server 'X' removed" confirmation — would leave stale windows
// pointing at a client this dialog no longer has any way to know
// should be torn down. The caller (App::showConnectionDialog()) is
// expected to close every such window and drop the server from its own
// settings_ right here, not wait for this dialog to close at all.
using ServerRemovedCallback = std::function<void(const std::string& name)>;

// Creates the "Connection" dialog pre-filled with `current`'s values,
// and populates `fields` with pointers to each individual field. Purely
// local (settings.json) state — no RPC call involved, unlike
// ServerSettingsDialog.
TDialog* createConnectionDialog(const AppSettings& current, ConnectionDialogFields& fields,
                                 ServerRemovedCallback onServerRemoved,
                                 ServerSavedCallback onServerSaved);

// Call after execView() == cmOK, BEFORE destroy(dialog) (otherwise the
// pointers in `fields` are no longer valid). Only reads refreshInterval
// and language now — the server actually shown (name, host, port,
// user, password) was already saved by ServerSavedCallback the moment
// Save/OK succeeded (see the button's own comment), same as a removal
// already happened by the time this runs (see ServerRemovedCallback).
// If the refresh interval isn't numeric (or is <= 0), keeps the
// previous value.
AppSettings connectionDialogResult(const ConnectionDialogFields& fields, const AppSettings& current);
