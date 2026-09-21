#include "ui/App.h"
#include "ui/Strings.h"
#include "Config.h"
#include "cli/Cli.h"

int main(int argc, char** argv) {
    AppSettings settings = loadSettings();

    // Needs to happen BEFORE constructing App: initMenuBar()/
    // initStatusLine() (static) get invoked while TApplication's base
    // classes are being constructed, so they already read the global
    // language state at that point (see the comment in App.h). The CLI
    // path also wants the language set before it runs, so it can print
    // translated messages too.
    setLanguage(settings.language);

    // Any command-line argument switches to the non-interactive CLI
    // (see cli/Cli.cpp); with none, launch the interactive TUI.
    if (argc > 1) {
        return runCli(argc, argv, settings);
    }

    App app(settings);
    app.run();
    app.shutDown();
    return 0;
}
