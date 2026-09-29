#pragma once

#define Uses_TDialog
#include <tvision/tv.h>

// Simple informational dialog: app name, version (see Version.h),
// copyright (current year, computed at runtime — not hardcoded, so it
// doesn't need updating every January), and the project's repository
// URL. Just an OK/Close button, nothing to configure.
TDialog* createAboutDialog();
