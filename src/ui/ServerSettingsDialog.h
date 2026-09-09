#pragma once

#define Uses_TDialog
#define Uses_TInputLine
#define Uses_TCheckBoxes
#include <tvision/tv.h>
#include "../rpc/TransmissionClient.h"

// Direct pointers to the Server Settings dialog's fields — same reason
// as ConnectionDialogFields: TGroup::insert() puts each new view at the
// HEAD of the child list, so scanning it afterwards gives the reverse
// of insertion order rather than a way to "find" fields reliably.
struct ServerSettingsDialogFields {
    // Global (session-wide) speed limits, live on the Transmission
    // daemon rather than in our own settings.json — see
    // TransmissionClient::getSessionLimits()/setSessionLimits().
    TCheckBoxes* globalLimitCheckboxes = nullptr; // bit 0 = download, bit 1 = upload
    TInputLine* globalDownloadLimit = nullptr;
    TInputLine* globalUploadLimit = nullptr;

    // "Speed Limit" (alt-speed / turtle) mode's own limits — always
    // editable regardless of whether that mode is currently active
    // (there's no enable checkbox for it here; see SessionLimits'
    // own comment on why).
    TInputLine* altSpeedDownloadLimit = nullptr;
    TInputLine* altSpeedUploadLimit = nullptr;
};

// Creates the "Server Configuration" dialog pre-filled with
// `sessionLimits` (fetched via TransmissionClient::getSessionLimits()
// before calling this — a live RPC call), and populates `fields` with
// pointers to each individual field.
TDialog* createServerSettingsDialog(const SessionLimits& sessionLimits,
                                     ServerSettingsDialogFields& fields);

// Call after execView() == cmOK, BEFORE destroy(dialog) (otherwise the
// pointers in `fields` are no longer valid).
SessionLimits serverSettingsDialogResult(const ServerSettingsDialogFields& fields);
