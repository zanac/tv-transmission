#pragma once

#define Uses_TDialog
#include <tvision/tv.h>
#include "../rpc/TransmissionClient.h"

// Creates the "Session Statistics" dialog — current-session and
// all-time (cumulative) transfer totals, active time, and daemon start
// count (see TransmissionClient::getSessionStats()). Purely
// informational: no field here round-trips back into any setting, so
// unlike ServerSettingsDialog there's no accompanying "...Result()"
// function or Fields struct to read afterward — just "Refresh" (re-runs
// the same RPC call and updates every label in place, the same
// TResultLabel-based approach ServerSettingsDialog's own "Test port"/
// "Update blocklist" already use) and "Close". `client` is held by
// reference for the dialog's own lifetime, same reason as
// ServerSettingsDialog's own.
TDialog* createSessionStatsDialog(const SessionStats& initial, TransmissionClient& client);
