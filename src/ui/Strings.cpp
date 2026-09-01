#include "Strings.h"

namespace {
Language g_language = Language::English;
} // namespace

void setLanguage(Language lang) { g_language = lang; }
Language currentLanguage() { return g_language; }

const char* tr(Str id) {
    const bool it = (g_language == Language::Italian);

    // An explicit switch, not a position-indexed table: after the
    // Settings dialog bug (fields scrambled by an index mismatch) I'd
    // rather have an explicit, at-a-glance-verifiable id->text mapping,
    // even if it's more verbose.
    switch (id) {
        case Str::MenuTorrent:   return "~T~orrent";
        case Str::MenuAdd:       return it ? "~A~ggiungi..."   : "~A~dd...";
        case Str::MenuStart:     return it ? "~S~tart"         : "~S~tart";
        case Str::MenuStop:      return it ? "S~t~op"          : "S~t~op";
        case Str::MenuRemove:    return it ? "~R~imuovi"       : "~R~emove";
        case Str::MenuSettings:  return it ? "~I~mpostazioni..." : "S~e~ttings...";
        case Str::MenuQuit:      return it ? "~E~sci"          : "~Q~uit";

        // Standard tvision window-management menu: we just label items
        // that send tvision's own standard commands (cmZoom, cmNext,
        // cmClose, cmTile, cmCascade) — the logic already lives in
        // tvision itself (TWindow/TDeskTop/TApplication).
        case Str::MenuWindow:        return it ? "~F~inestra" : "~W~indow";
        case Str::MenuWindowZoom:    return "~Z~oom";
        case Str::MenuWindowNext:    return it ? "~S~uccessiva" : "~N~ext";
        case Str::MenuWindowClose:   return it ? "~C~hiudi"     : "~C~lose";
        case Str::MenuWindowTile:    return it ? "~R~iquadra"   : "~T~ile";
        case Str::MenuWindowCascade: return it ? "Casc~a~ta"    : "C~a~scade";
        case Str::MenuWindowList:    return it ? "~E~lenco finestre" : "Window ~l~ist";
        // Separate from MenuWindowList on purpose: menu/status labels use
        // ~x~ markup to underline a hotkey letter, which only TMenuItem/
        // TStatusItem/TButton interpret. A TDialog title does NOT strip
        // it, so reusing the menu label here would show literal tildes
        // in the title bar.
        case Str::DialogTitleWindowList: return it ? "Elenco finestre" : "Window list";

        case Str::StatusAdd:      return it ? "~F2~ Aggiungi"      : "~F2~ Add";
        case Str::StatusStart:    return "~F5~ Start";
        case Str::StatusStop:     return "~F6~ Stop";
        case Str::StatusSettings: return it ? "~F9~ Impostazioni" : "~F9~ Settings";
        case Str::StatusQuit:     return it ? "~Alt-X~ Esci"      : "~Alt-X~ Quit";

        case Str::WindowTitleTorrentList: return it ? "Torrent" : "Torrents";

        case Str::DialogTitleAddTorrent: return it ? "Aggiungi torrent" : "Add torrent";
        case Str::LabelAddTorrentUrl:
            return it ? "Magnet link, URL .torrent o path locale:"
                      : "Magnet link, .torrent URL or local path:";

        case Str::ButtonOK:     return "OK";
        case Str::ButtonCancel: return it ? "Annulla" : "Cancel";

        case Str::DialogTitleSettings: return it ? "Impostazioni" : "Settings";
        case Str::LabelRefreshSeconds:
            return it ? "Refresh (secondi):" : "Refresh (seconds):";
        case Str::LabelHost:
            return it ? "Host Transmission:" : "Transmission host:";
        case Str::LabelPort:
            return it ? "Porta RPC:" : "RPC port:";
        case Str::LabelUser:
            return it ? "Utente (opzionale):" : "User (optional):";
        case Str::LabelPassword:
            return it ? "Password (opzionale):" : "Password (optional):";
        case Str::LabelLanguage:
            return it ? "Lingua:" : "Language:";
        // Native language names: identical regardless of the currently
        // selected language, so they stay recognizable to someone
        // looking for their own language in the list.
        case Str::LanguageEnglish: return "English";
        case Str::LanguageItalian: return "Italiano";

        case Str::WindowTitleDetails: return it ? "Dettagli torrent" : "Torrent details";
        case Str::LabelName:      return it ? "Nome: %s"            : "Name: %s";
        case Str::LabelSize:      return it ? "Dimensione: %s"      : "Size: %s";
        case Str::LabelCompleted: return it ? "Completato: %.1f%%" : "Completed: %.1f%%";
        case Str::LabelDownload:  return "Download: %.1f KB/s";
        case Str::LabelUpload:    return "Upload: %.1f KB/s";
        case Str::LabelStatus:    return it ? "Stato: %s"           : "Status: %s";
        case Str::LabelError:     return it ? "Errore: %s"          : "Error: %s";
        case Str::LabelId:        return "ID: %d";
        case Str::LabelAdded:     return it ? "Aggiunto: %s"        : "Added: %s";

        case Str::TorrentStatusStopped:      return it ? "Fermo"                    : "Stopped";
        case Str::TorrentStatusCheckWait:    return it ? "In attesa di verifica"     : "Queued for check";
        case Str::TorrentStatusChecking:     return it ? "Verifica in corso"         : "Checking";
        case Str::TorrentStatusDownloadWait: return it ? "In attesa di download"     : "Queued for download";
        case Str::TorrentStatusDownloading:  return it ? "Download in corso"         : "Downloading";
        case Str::TorrentStatusSeedWait:     return it ? "In attesa di seeding"      : "Queued for seeding";
        case Str::TorrentStatusSeeding:      return it ? "Seeding"                   : "Seeding";
        case Str::TorrentStatusUnknown:      return it ? "Sconosciuto"               : "Unknown";

        // Torrent list column headers. "Download"/"Upload" stay the same
        // in both languages: it's the same convention Transmission
        // itself uses in its own Italian UI.
        case Str::HeaderName:     return it ? "Nome"   : "Name";
        case Str::HeaderDone:     return it ? "Compl." : "Done";
        case Str::HeaderSize:     return it ? "Dim."   : "Size";
        case Str::HeaderDownload: return "Download";
        case Str::HeaderUpload:   return "Upload";
        case Str::HeaderId:       return "ID";
        case Str::HeaderStatus:   return it ? "Stato" : "Status";
        case Str::HeaderAdded:    return it ? "Aggiunto" : "Added";

        case Str::CliUsage:
            return it ?
"Uso: tv-transmission [opzioni globali] <comando> [argomenti]\n"
"\n"
"Senza argomenti, avvia la TUI interattiva.\n"
"\n"
"Comandi:\n"
"  list                        Elenca tutti i torrent\n"
"  add <magnet|url|path>       Aggiunge un nuovo torrent\n"
"  start <id>                  Avvia (riprende) un torrent\n"
"  stop <id>                   Ferma (mette in pausa) un torrent\n"
"  remove <id> [--delete-data] Rimuove un torrent (opzionalmente cancellando i dati locali)\n"
"  help                        Mostra questo messaggio\n"
"\n"
"Opzioni globali:\n"
"  --host <host>       Host RPC di Transmission (default: dalle impostazioni salvate)\n"
"  --port <porta>      Porta RPC di Transmission (default: dalle impostazioni salvate)\n"
"  --user <utente>     Utente RPC (default: dalle impostazioni salvate)\n"
"  --password <pass>   Password RPC (default: dalle impostazioni salvate)\n"
"  -h, --help          Mostra questo messaggio\n"
"\n"
"Di default host/porta/utente/password vengono letti dal file delle\n"
"impostazioni salvate (~/.config/tv-transmission/settings.json), lo\n"
"stesso impostato dalla finestra Impostazioni della TUI, quindi non\n"
"serve passarli a ogni comando. Ognuna delle opzioni --host/--port/\n"
"--user/--password sovrascrive solo quel valore."
            :
"Usage: tv-transmission [global options] <command> [args]\n"
"\n"
"Without arguments, launches the interactive TUI.\n"
"\n"
"Commands:\n"
"  list                        List all torrents\n"
"  add <magnet|url|path>       Add a new torrent\n"
"  start <id>                  Start (resume) a torrent\n"
"  stop <id>                   Stop (pause) a torrent\n"
"  remove <id> [--delete-data] Remove a torrent (optionally deleting local data)\n"
"  help                        Show this message\n"
"\n"
"Global options:\n"
"  --host <host>       Transmission RPC host (default: from saved settings)\n"
"  --port <port>       Transmission RPC port (default: from saved settings)\n"
"  --user <user>       RPC username (default: from saved settings)\n"
"  --password <pass>   RPC password (default: from saved settings)\n"
"  -h, --help          Show this message\n"
"\n"
"By default host/port/user/password are read from the saved settings\n"
"file (~/.config/tv-transmission/settings.json), the same one set from\n"
"the TUI's Settings window, so you don't need to pass them on every\n"
"command. Any of --host/--port/--user/--password overrides just that\n"
"one value.";

        case Str::CliErrorMissingArgument: return it ? "Argomento mancante: %s" : "Missing argument: %s";
        case Str::CliErrorUnknownCommand:  return it ? "Comando sconosciuto: %s" : "Unknown command: %s";
        case Str::CliErrorInvalidId:       return it ? "ID torrent non valido: %s" : "Invalid torrent id: %s";
        case Str::CliListEmpty:           return it ? "Nessun torrent." : "No torrents.";
        case Str::CliAddSuccess:    return it ? "Torrent aggiunto." : "Torrent added.";
        case Str::CliAddFailure:    return it ? "Aggiunta del torrent non riuscita." : "Failed to add torrent.";
        case Str::CliStartSuccess:  return it ? "Torrent avviato." : "Torrent started.";
        case Str::CliStartFailure:  return it ? "Avvio del torrent non riuscito." : "Failed to start torrent.";
        case Str::CliStopSuccess:   return it ? "Torrent fermato." : "Torrent stopped.";
        case Str::CliStopFailure:   return it ? "Arresto del torrent non riuscito." : "Failed to stop torrent.";
        case Str::CliRemoveSuccess: return it ? "Torrent rimosso." : "Torrent removed.";
        case Str::CliRemoveFailure: return it ? "Rimozione del torrent non riuscita." : "Failed to remove torrent.";
    }
    return ""; // unhandled id: shouldn't happen (exhaustive switch above)
}

const char* trTorrentStatus(int status) {
    switch (status) {
        case 0: return tr(Str::TorrentStatusStopped);
        case 1: return tr(Str::TorrentStatusCheckWait);
        case 2: return tr(Str::TorrentStatusChecking);
        case 3: return tr(Str::TorrentStatusDownloadWait);
        case 4: return tr(Str::TorrentStatusDownloading);
        case 5: return tr(Str::TorrentStatusSeedWait);
        case 6: return tr(Str::TorrentStatusSeeding);
        default: return tr(Str::TorrentStatusUnknown);
    }
}
