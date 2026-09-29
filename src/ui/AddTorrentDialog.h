#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#include <tvision/tv.h>
#include <string>
#include "../rpc/TransmissionClient.h"

// Creates the "Add torrent" dialog: destination folder (a label, shown
// read-only, plus a "Change..." button — never a directly-editable
// field here, see App::showAddTorrentDialog()'s own comment on why
// changing it always goes through TFolderBrowserDialog instead) and
// its own free space (a label plus a "Verify" button, both fetched
// once automatically as soon as the dialog opens, so there's a real
// number to look at immediately rather than needing an extra click
// just to see one the first time), then the existing magnet link/
// .torrent URL/local path field and its own "Browse..." button.
// Confirming with OK returns cmOK as usual; Browse ends the dialog with
// cmYes (repurposing tvision's own built-in Yes command, which
// TDialog's own handleEvent already turns into endModal() while modal
// — no custom subclass needed for that one specifically). "Change..."
// ends it with its own dedicated cmChangeFolder rather than reusing
// cmNo the same way — deliberately NOT shared with anything else,
// because every TButton re-checks commandEnabled(its own command) on
// every process-wide cmCommandSetChanged broadcast (tvision's own
// tbutton.cpp): a command shared with other buttons elsewhere gets
// silently re-enabled the moment anything else in the app touches the
// global command set, regardless of this dialog's own intent — found
// directly, reported with the button still opening the folder browser
// on click despite LOOKING disabled. Given that, "Change..." itself is
// hidden outright for a remote daemon (see isLocalHost() in the .cpp:
// only meaningful against a local one) rather than left visible but
// disabled — a greyed button that's still clearly there invites
// clicking it anyway; a hidden one raises no such question at all.
//
// 203, not some low unused-looking number: tvision's own built-in
// commands occupy most of 0-102 (cmClose is 4, cmMenu is 3, and so on
// — see views.h), 301, and 500 upward, none of it obvious from the
// name alone. This one was first tried at 4 and collided with
// tvision's own cmClose — the button drew correctly enabled but a
// click visibly did nothing (not even the dialog closing, which is
// what cmClose would suggest — collisions inside tvision's own
// internals don't necessarily fail the way you'd expect from the name
// of whatever they collided with). 200+ is clear of all of tvision's
// own ranges and is what every other local command in this project
// already uses.
constexpr ushort cmChangeFolder = 203;
TDialog* createAddTorrentDialog(TInputLine*& urlField, TransmissionClient& client,
                                 const std::string& initialValue = "",
                                 const std::string& initialDestination = "");

// Call after execView() == cmOK, BEFORE destroy(dialog).
std::string addTorrentDialogResult(TInputLine* urlField);

// Reads back whatever destination the dialog was showing at the
// moment it closed, regardless of which command ended it — needed on
// cmYes/cmChangeFolder too (not just cmOK), so a destination already
// chosen survives the Browse/Change reopen cycle instead of resetting
// to the server's own default every time.
std::string addTorrentDialogDestination(TDialog* dialog);
