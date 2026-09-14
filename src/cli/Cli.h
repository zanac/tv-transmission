#pragma once
#include "../AppSettings.h"

// Runs the command-line interface. `argc`/`argv` are the same as
// main()'s (argv[0] is the program name). `settings` should already be
// loaded via loadSettings() so CLI commands use the saved
// host/port/user/password unless overridden by --host/--port/--user/
// --password flags. Returns the process exit code.
int runCli(int argc, char** argv, AppSettings settings);
