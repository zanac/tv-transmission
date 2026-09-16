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
// cmYes, "Change..." with cmNo (repurposing two of tvision's own
// built-in Yes/No commands, which TDialog's own handleEvent already
// turns into endModal() while modal — no custom subclass needed for
// either one specifically, only "Verify" needs one, since that's the
// one thing here that acts on `client` WHILE the dialog stays open
// rather than only once it closes).
//
// `client` is held by reference for the dialog's own lifetime, the
// same reason ServerSettingsDialog and SessionStatsDialog both do:
// "Verify" calls straight into it, and the initial destination/free-
// space fetch on open does too.
//
// The caller is expected to destroy this dialog on cmYes or cmNo (not
// only cmOK/cmCancel), THEN open the relevant follow-up dialog directly
// from the application (TFileDialog for Browse, TFolderBrowserDialog
// for Change — both one level of nesting, not two) and, once that's
// done, recreate this same dialog with `initialValue`/
// `initialDestination` set so the user still sees/can edit the URL and
// still sees whatever destination was already chosen before confirming
// — see App::showAddTorrentDialog()'s own comment on the modal-nesting
// bug this avoids, the same one that already ruled out nesting
// TFileDialog directly inside this dialog for Browse.
TDialog* createAddTorrentDialog(TInputLine*& urlField, TransmissionClient& client,
                                 const std::string& initialValue = "",
                                 const std::string& initialDestination = "");

// Call after execView() == cmOK, BEFORE destroy(dialog).
std::string addTorrentDialogResult(TInputLine* urlField);

// Reads back whatever destination the dialog was showing at the
// moment it closed, regardless of which command ended it — needed on
// cmYes/cmNo too (not just cmOK), so a destination already chosen
// survives the Browse/Change reopen cycle instead of resetting to the
// server's own default every time.
std::string addTorrentDialogDestination(TDialog* dialog);
