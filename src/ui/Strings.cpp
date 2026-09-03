#include "Strings.h"

namespace {
Language g_language = Language::English;

// Five-way pick instead of the old English/Italian ternary — every
// call site below passes (english, italian, french, german, spanish) in
// that fixed order, which the case list groups by column for the same
// reason the old ternary style did: an unhandled string here is a
// mistranslation, not a build error, so keeping five aligned columns
// makes a missed/misordered language visually obvious on review.
const char* pick(const char* en, const char* it, const char* fr, const char* de, const char* es) {
    switch (g_language) {
        case Language::Italian: return it;
        case Language::French:  return fr;
        case Language::German:  return de;
        case Language::Spanish: return es;
        default:                return en;
    }
}
} // namespace

void setLanguage(Language lang) { g_language = lang; }
Language currentLanguage() { return g_language; }

const char* tr(Str id) {
    // An explicit switch, not a position-indexed table: after the
    // Settings dialog bug (fields scrambled by an index mismatch) I'd
    // rather have an explicit, at-a-glance-verifiable id->text mapping,
    // even if it's more verbose.
    switch (id) {
        case Str::MenuTorrent:   return "~T~orrent"; // same word in all five languages
        case Str::MenuAdd:       return pick("~A~dd...", "~A~ggiungi...", "~A~jouter...", "~H~inzufügen...", "~A~ñadir...");
        case Str::MenuStart:     return pick("~S~tart", "~S~tart", "~D~émarrer", "~S~tarten", "~I~niciar");
        case Str::MenuStop:      return pick("S~t~op", "S~t~op", "Arrê~t~er", "S~t~oppen", "~D~etener");
        case Str::MenuRemove:    return pick("~R~emove", "~R~imuovi", "~S~upprimer", "~E~ntfernen", "~E~liminar");
        case Str::MenuSettings:  return pick("S~e~ttings...", "~I~mpostazioni...", "~P~aramètres...", "Ei~n~stellungen...", "~C~onfiguración...");
        case Str::MenuQuit:      return pick("~Q~uit", "~E~sci", "~Q~uitter", "~B~eenden", "~S~alir");
        case Str::MenuVerify:      return pick("~V~erify", "~V~erifica", "~V~érifier", "~P~rüfen", "~V~erificar");
        case Str::MenuReannounce:  return pick("Reannoun~c~e", "Ricontatta tra~c~ker", "Réannon~c~er", "Tracker neu ~a~nfragen", "Reanun~c~iar");
        case Str::MenuStartNow:    return pick("Start ~N~ow", "Avvia s~u~bito", "Démarrer ~m~aintenant", "~J~etzt starten", "Iniciar ~a~hora");
        case Str::MenuShowDetails: return pick("~D~etails", "~D~ettagli", "Détai~l~s", "~D~etails", "Detal~l~es");
        case Str::MenuDeleteWithData:
            return pick("Delete (~w~ith files)", "Elimina (con ~f~ile)", "~E~ffacer (avec fichiers)",
                        "~L~öschen (mit Dateien)", "~B~orrar (con archivos)");
        case Str::ConfirmRemoveTorrent:
            return pick("Remove '%s' from the list? Files on disk will be kept.",
                        "Rimuovere '%s' dalla lista? I file su disco resteranno.",
                        "Retirer '%s' de la liste ? Les fichiers sur le disque seront conservés.",
                        "'%s' aus der Liste entfernen? Die Dateien auf der Festplatte bleiben erhalten.",
                        "¿Quitar '%s' de la lista? Los archivos en el disco se conservarán.");
        case Str::ConfirmDeleteTorrentWithData:
            return pick("Delete '%s' AND its files on disk? This cannot be undone.",
                        "Eliminare '%s' E i suoi file su disco? L'operazione non si puo' annullare.",
                        "Supprimer '%s' ET ses fichiers sur le disque ? Cette action est irréversible.",
                        "'%s' UND die zugehörigen Dateien löschen? Dies kann nicht rückgängig gemacht werden.",
                        "¿Eliminar '%s' Y sus archivos en el disco? Esta acción no se puede deshacer.");

        // Standard tvision window-management menu: we just label items
        // that send tvision's own standard commands (cmZoom, cmNext,
        // cmClose, cmTile, cmCascade) — the logic already lives in
        // tvision itself (TWindow/TDeskTop/TApplication).
        case Str::MenuWindow:        return pick("~W~indow", "~F~inestra", "~F~enêtre", "~F~enster", "~V~entana");
        case Str::MenuWindowZoom:    return "~Z~oom"; // same word in all five languages
        case Str::MenuWindowNext:    return pick("~N~ext", "~S~uccessiva", "~S~uivante", "~N~ächstes", "~S~iguiente");
        case Str::MenuWindowClose:   return pick("~C~lose", "~C~hiudi", "~F~ermer", "~S~chließen", "~C~errar");
        case Str::MenuWindowTile:    return pick("~T~ile", "~R~iquadra", "~M~osaïque", "~K~acheln", "~M~osaico");
        case Str::MenuWindowCascade: return pick("C~a~scade", "Casc~a~ta", "Casc~a~de", "Kas~a~de", "Casc~a~da");
        case Str::MenuWindowList:    return pick("Window ~l~ist", "~E~lenco finestre", "~L~iste des fenêtres",
                                                  "~F~ensterliste", "~L~ista de ventanas");
        // Separate from MenuWindowList on purpose: menu/status labels use
        // ~x~ markup to underline a hotkey letter, which only TMenuItem/
        // TStatusItem/TButton interpret. A TDialog title does NOT strip
        // it, so reusing the menu label here would show literal tildes
        // in the title bar.
        case Str::DialogTitleWindowList:
            return pick("Window list", "Elenco finestre", "Liste des fenêtres", "Fensterliste", "Lista de ventanas");

        case Str::MenuHelp:  return pick("~H~elp", "~A~iuto", "~A~ide", "~H~ilfe", "~A~yuda");
        case Str::MenuAbout: return pick("~A~bout...", "~I~nfo su...", "À ~p~ropos...", "Ü~b~er...", "~A~cerca de...");
        case Str::DialogTitleAbout:
            return pick("About", "Info su", "À propos", "Über", "Acerca de");
        case Str::LabelAboutVersion:
            return pick("Version: %s", "Versione: %s", "Version : %s", "Version: %s", "Versión: %s");
        case Str::LabelAboutCopyright:
            return "Copyright © %d %s"; // same convention in all five languages

        case Str::StatusAdd:      return pick("~F2~ Add", "~F2~ Aggiungi", "~F2~ Ajouter", "~F2~ Hinzufügen", "~F2~ Añadir");
        case Str::StatusStart:    return "~F5~ Start"; // same word in all five languages
        case Str::StatusStop:     return pick("~F6~ Stop", "~F6~ Stop", "~F6~ Arrêter", "~F6~ Stopp", "~F6~ Detener");
        case Str::StatusSettings: return pick("~F9~ Settings", "~F9~ Impostazioni", "~F9~ Paramètres", "~F9~ Einstellungen", "~F9~ Config.");
        case Str::StatusQuit:     return pick("~Alt-X~ Quit", "~Alt-X~ Esci", "~Alt-X~ Quitter", "~Alt-X~ Beenden", "~Alt-X~ Salir");

        case Str::WindowTitleTorrentList:
            return pick("Torrents", "Torrent", "Torrents", "Torrents", "Torrents");

        case Str::DialogTitleAddTorrent:
            return pick("Add torrent", "Aggiungi torrent", "Ajouter un torrent", "Torrent hinzufügen", "Añadir torrent");
        case Str::LabelAddTorrentUrl:
            return pick("Magnet link, .torrent URL or local path:",
                        "Magnet link, URL .torrent o path locale:",
                        "Lien magnet, URL .torrent ou chemin local :",
                        "Magnet-Link, .torrent-URL oder lokaler Pfad:",
                        "Enlace magnet, URL .torrent o ruta local:");
        case Str::MsgTorrentDuplicate:
            return pick("This torrent was already in the list.",
                        "Questo torrent era già presente nella lista.",
                        "Ce torrent était déjà dans la liste.",
                        "Dieser Torrent war bereits in der Liste.",
                        "Este torrent ya estaba en la lista.");
        case Str::MsgTorrentAddFailed:
            return pick("Failed to add the torrent:\n%s",
                        "Aggiunta del torrent non riuscita:\n%s",
                        "Échec de l'ajout du torrent :\n%s",
                        "Hinzufügen des Torrents fehlgeschlagen:\n%s",
                        "No se pudo añadir el torrent:\n%s");

        case Str::ButtonOK:     return "OK"; // same word in all five languages
        case Str::ButtonCancel: return pick("Cancel", "Annulla", "Annuler", "Abbrechen", "Cancelar");

        case Str::DialogTitleSettings:
            return pick("Settings", "Impostazioni", "Paramètres", "Einstellungen", "Configuración");
        case Str::LabelRefreshSeconds:
            return pick("Refresh (seconds):", "Refresh (secondi):", "Actualisation (secondes) :",
                        "Aktualisierung (Sekunden):", "Actualización (segundos):");
        case Str::LabelHost:
            return pick("Transmission host:", "Host Transmission:", "Hôte Transmission :",
                        "Transmission-Host:", "Host de Transmission:");
        case Str::LabelPort:
            return pick("RPC port:", "Porta RPC:", "Port RPC :", "RPC-Port:", "Puerto RPC:");
        case Str::LabelUser:
            return pick("User (optional):", "Utente (opzionale):", "Utilisateur (facultatif) :",
                        "Benutzer (optional):", "Usuario (opcional):");
        case Str::LabelPassword:
            return pick("Password (optional):", "Password (opzionale):", "Mot de passe (facultatif) :",
                        "Passwort (optional):", "Contraseña (opcional):");
        case Str::LabelLanguage:
            return pick("Language:", "Lingua:", "Langue :", "Sprache:", "Idioma:");
        // Native language names: identical regardless of the currently
        // selected language, so they stay recognizable to someone
        // looking for their own language in the list.
        case Str::LanguageEnglish: return "English";
        case Str::LanguageItalian: return "Italiano";
        case Str::LanguageFrench:  return "Français";
        case Str::LanguageGerman:  return "Deutsch";
        case Str::LanguageSpanish: return "Español";

        case Str::WindowTitleDetails:
            return pick("Torrent details", "Dettagli torrent", "Détails du torrent", "Torrent-Details", "Detalles del torrent");
        case Str::LabelName:
            return pick("Name: %s", "Nome: %s", "Nom : %s", "Name: %s", "Nombre: %s");
        case Str::LabelSize:
            return pick("Size: %s", "Dimensione: %s", "Taille : %s", "Größe: %s", "Tamaño: %s");
        case Str::LabelCompleted:
            return pick("Completed: %.1f%%", "Completato: %.1f%%", "Terminé : %.1f%%",
                        "Abgeschlossen: %.1f%%", "Completado: %.1f%%");
        case Str::LabelDownload:  return "Download: %.1f KB/s"; // "Download" is the same word in all five here
        case Str::LabelUpload:    return "Upload: %.1f KB/s";   // same reasoning
        case Str::LabelStatus:
            return pick("Status: %s", "Stato: %s", "État : %s", "Status: %s", "Estado: %s");
        case Str::LabelError:
            return pick("Error: %s", "Errore: %s", "Erreur : %s", "Fehler: %s", "Error: %s");
        case Str::LabelId:        return "ID: %d"; // same in all five
        case Str::LabelAdded:
            return pick("Added: %s", "Aggiunto: %s", "Ajouté : %s", "Hinzugefügt: %s", "Añadido: %s");

        case Str::LabelLocation:
            return pick("Location: %s", "Posizione: %s", "Emplacement : %s", "Speicherort: %s", "Ubicación: %s");
        case Str::LabelPrivacyPublic:
            return pick("Privacy: public torrent", "Privacy: torrent pubblico", "Confidentialité : torrent public",
                        "Datenschutz: öffentlicher Torrent", "Privacidad: torrent público");
        case Str::LabelPrivacyPrivate:
            return pick("Privacy: private torrent", "Privacy: torrent privato", "Confidentialité : torrent privé",
                        "Datenschutz: privater Torrent", "Privacidad: torrent privado");
        case Str::LabelMagnet:    return "Magnet: %s"; // same word in all five
        case Str::LabelPieces:
            return pick("Pieces: %lld of %s", "Sezioni: %lld da %s", "Morceaux : %lld de %s",
                        "Teile: %lld von %s", "Piezas: %lld de %s");

        case Str::LabelAvailable:
            return pick("Available: %.1f%%", "Disponibile: %.1f%%", "Disponible : %.1f%%",
                        "Verfügbar: %.1f%%", "Disponible: %.1f%%");
        case Str::LabelDownloadedTotal:
            return pick("Downloaded: %s", "Scaricato: %s", "Téléchargé : %s", "Heruntergeladen: %s", "Descargado: %s");
        case Str::LabelUploadedTotal:
            return pick("Uploaded: %s (Ratio: %.2f)", "Inviato: %s (Ratio: %.2f)", "Envoyé : %s (Ratio : %.2f)",
                        "Hochgeladen: %s (Verhältnis: %.2f)", "Subido: %s (Ratio: %.2f)");
        case Str::LabelAverageSpeed:
            return pick("Average speed: %s", "Velocità media: %s", "Vitesse moyenne : %s",
                        "Durchschnittsgeschwindigkeit: %s", "Velocidad media: %s");

        case Str::LabelLastActivity:
            return pick("Last activity: %s", "Ultima attività: %s", "Dernière activité : %s",
                        "Letzte Aktivität: %s", "Última actividad: %s");

        case Str::LabelTimeDownloading:
            return pick("Downloading: %s", "In download: %s", "Téléchargement : %s", "Herunterladen: %s", "Descargando: %s");
        case Str::LabelTimeSeeding:
            return pick("Seeding: %s", "In seeding: %s", "Partage : %s", "Seeding: %s", "Compartiendo: %s");

        case Str::LabelSpeedLimitSection:
            return pick("Speed limit for this torrent:", "Limite di velocità per questo torrent:",
                        "Limite de vitesse pour ce torrent :", "Geschwindigkeitslimit für diesen Torrent:",
                        "Límite de velocidad para este torrent:");
        case Str::CheckLimitDownload:
            return pick("Limit download", "Limita download", "Limiter le téléchargement",
                        "Download begrenzen", "Limitar descarga");
        case Str::CheckLimitUpload:
            return pick("Limit upload", "Limita upload", "Limiter l'envoi", "Upload begrenzen", "Limitar subida");
        case Str::UnitKBs: return "KB/s"; // same in all five
        case Str::CheckHonorGlobalLimits:
            return pick("Honor global speed limits", "Rispetta i limiti globali",
                        "Respecter les limites globales", "Globale Limits berücksichtigen",
                        "Respetar los límites globales");
        case Str::ButtonApply: return pick("Apply", "Applica", "Appliquer", "Anwenden", "Aplicar");
        // Distinct from MenuWindowClose on purpose: same reason as
        // DialogTitleWindowList above (menu labels carry ~x~ hotkey
        // markup that a TButton here doesn't need and would show as
        // literal tildes if reused verbatim... actually TButton DOES
        // interpret ~x~, but keeping a separate plain string here avoids
        // any accidental coupling between this window's button and the
        // Window menu's Close item, which apply to different things
        // (this torrent's window vs. "whichever window is active").
        case Str::ButtonClose: return pick("Close", "Chiudi", "Fermer", "Schließen", "Cerrar");
        case Str::ButtonTrackers: return pick("Trackers...", "Tracker...", "Trackers...", "Tracker...", "Trackers...");
        case Str::ButtonRefresh: return pick("Refresh", "Aggiorna", "Actualiser", "Aktualisieren", "Actualizar");
        case Str::ButtonBrowse: return pick("Browse...", "Sfoglia...", "Parcourir...", "Durchsuchen...", "Examinar...");
        case Str::DialogTitleBrowseTorrent:
            return pick("Select a .torrent file", "Seleziona un file .torrent",
                        "Sélectionner un fichier .torrent", "Wähle eine .torrent-Datei",
                        "Selecciona un archivo .torrent");

        case Str::WindowTitleTrackerList:
            return pick("Trackers: %s", "Tracker: %s", "Trackers : %s", "Tracker: %s", "Trackers: %s");
        case Str::WindowTitleTrackerDetail:
            return pick("Tracker details", "Dettagli tracker", "Détails du tracker", "Tracker-Details", "Detalles del tracker");
        case Str::HeaderTrackerHost: return "Tracker"; // same word in all five
        case Str::HeaderTier:        return "Tier"; // kept as-is in all five (Transmission's own term)
        case Str::HeaderSeeders:     return "Seeders"; // same word in all five
        case Str::HeaderLeechers:    return "Leechers"; // same word in all five
        case Str::HeaderDownloaded:
            return pick("Downloaded", "Scaricati", "Téléchargé", "Heruntergeladen", "Descargado");
        case Str::HeaderTrackerStatus:
            return pick("Status", "Stato", "État", "Status", "Estado");
        case Str::TrackerStatusOk: return "OK"; // same in all five
        case Str::TrackerStatusError:
            return pick("Error", "Errore", "Erreur", "Fehler", "Error");
        case Str::ValueNotAvailable: return "N/A"; // same in all five
        case Str::LabelTrackerHost: return "Tracker: %s"; // same word in all five
        case Str::LabelTrackerTier: return "Tier: %d"; // same in all five
        case Str::LabelTrackerSeeders: return "Seeders: %s"; // same in all five
        case Str::LabelTrackerLeechers: return "Leechers: %s"; // same in all five
        case Str::LabelTrackerDownloaded:
            return pick("Downloaded: %s", "Scaricati: %s", "Téléchargé : %s", "Heruntergeladen: %s", "Descargado: %s");
        case Str::LabelTrackerLastAnnounce:
            return pick("Last announce: %s", "Ultimo annuncio: %s", "Dernière annonce : %s",
                        "Letzte Ankündigung: %s", "Último anuncio: %s");
        case Str::LabelTrackerNextAnnounce:
            return pick("Next announce: %s", "Prossimo annuncio: %s", "Prochaine annonce : %s",
                        "Nächste Ankündigung: %s", "Próximo anuncio: %s");
        case Str::LabelTrackerResult:
            return pick("Result: %s", "Risultato: %s", "Résultat : %s", "Ergebnis: %s", "Resultado: %s");

        case Str::LabelGlobalSpeedSection:
            return pick("Global speed limit (all torrents):", "Limite di velocità globale (tutti i torrent):",
                        "Limite de vitesse globale (tous les torrents) :",
                        "Globales Geschwindigkeitslimit (alle Torrents):",
                        "Límite de velocidad global (todos los torrents):");
        case Str::CheckGlobalLimitDownload:
            return pick("Limit download", "Limita download", "Limiter le téléchargement",
                        "Download begrenzen", "Limitar descarga");
        case Str::CheckGlobalLimitUpload:
            return pick("Limit upload", "Limita upload", "Limiter l'envoi", "Upload begrenzen", "Limitar subida");

        case Str::TorrentStatusStopped:
            return pick("Stopped", "Fermo", "Arrêté", "Gestoppt", "Detenido");
        case Str::TorrentStatusCheckWait:
            return pick("Queued for check", "In attesa di verifica", "En attente de vérification",
                        "Wartet auf Prüfung", "En espera de verificación");
        case Str::TorrentStatusChecking:
            return pick("Checking", "Verifica in corso", "Vérification en cours", "Wird geprüft", "Verificando");
        case Str::TorrentStatusDownloadWait:
            return pick("Queued for download", "In attesa di download", "En attente de téléchargement",
                        "Wartet auf Download", "En espera de descarga");
        case Str::TorrentStatusDownloading:
            return pick("Downloading", "Download in corso", "Téléchargement en cours", "Wird heruntergeladen", "Descargando");
        case Str::TorrentStatusSeedWait:
            return pick("Queued for seeding", "In attesa di seeding", "En attente de partage",
                        "Wartet auf Seeding", "En espera de compartir");
        case Str::TorrentStatusSeeding:
            return pick("Seeding", "Seeding", "Partage", "Seeding", "Compartiendo");
        case Str::TorrentStatusUnknown:
            return pick("Unknown", "Sconosciuto", "Inconnu", "Unbekannt", "Desconocido");

        // Torrent list column headers. "Download"/"Upload" stay the same
        // in all five languages: it's the same convention Transmission
        // itself uses in its own translated UIs.
        case Str::HeaderName:
            return pick("Name", "Nome", "Nom", "Name", "Nombre");
        case Str::HeaderDone:
            return pick("Done", "Compl.", "Fait", "Fertig", "Hecho");
        case Str::HeaderSize:
            return pick("Size", "Dim.", "Taille", "Größe", "Tamaño");
        case Str::HeaderDownload: return "Download"; // same word in all five
        case Str::HeaderUpload:   return "Upload"; // same word in all five
        case Str::HeaderId:       return "ID"; // same in all five
        case Str::HeaderStatus:
            return pick("Status", "Stato", "État", "Status", "Estado");
        case Str::HeaderAdded:
            return pick("Added", "Aggiunto", "Ajouté", "Hinzugefügt", "Añadido");

        case Str::CliUsage:
            switch (g_language) {
                case Language::Italian: return
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
"--user/--password sovrascrive solo quel valore.";
                case Language::French: return
"Usage : tv-transmission [options globales] <commande> [arguments]\n"
"\n"
"Sans argument, lance la TUI interactive.\n"
"\n"
"Commandes :\n"
"  list                        Liste tous les torrents\n"
"  add <magnet|url|chemin>     Ajoute un nouveau torrent\n"
"  start <id>                  Démarre (reprend) un torrent\n"
"  stop <id>                   Arrête (met en pause) un torrent\n"
"  remove <id> [--delete-data] Supprime un torrent (en effaçant éventuellement les données locales)\n"
"  help                        Affiche ce message\n"
"\n"
"Options globales :\n"
"  --host <hôte>       Hôte RPC de Transmission (par défaut : paramètres enregistrés)\n"
"  --port <port>       Port RPC de Transmission (par défaut : paramètres enregistrés)\n"
"  --user <utilisateur> Utilisateur RPC (par défaut : paramètres enregistrés)\n"
"  --password <mdp>    Mot de passe RPC (par défaut : paramètres enregistrés)\n"
"  -h, --help          Affiche ce message\n"
"\n"
"Par défaut, hôte/port/utilisateur/mot de passe sont lus depuis le\n"
"fichier de paramètres enregistrés (~/.config/tv-transmission/settings.json),\n"
"le même que celui défini depuis la fenêtre Paramètres de la TUI, donc\n"
"inutile de les repasser à chaque commande. Chacune des options\n"
"--host/--port/--user/--password ne remplace que cette valeur-là.";
                case Language::German: return
"Verwendung: tv-transmission [globale Optionen] <Befehl> [Argumente]\n"
"\n"
"Ohne Argumente wird die interaktive TUI gestartet.\n"
"\n"
"Befehle:\n"
"  list                        Listet alle Torrents auf\n"
"  add <magnet|url|pfad>       Fügt einen neuen Torrent hinzu\n"
"  start <id>                  Startet (setzt fort) einen Torrent\n"
"  stop <id>                   Stoppt (pausiert) einen Torrent\n"
"  remove <id> [--delete-data] Entfernt einen Torrent (optional inklusive lokaler Daten)\n"
"  help                        Zeigt diese Meldung an\n"
"\n"
"Globale Optionen:\n"
"  --host <host>       Transmission-RPC-Host (Standard: aus gespeicherten Einstellungen)\n"
"  --port <port>       Transmission-RPC-Port (Standard: aus gespeicherten Einstellungen)\n"
"  --user <benutzer>   RPC-Benutzername (Standard: aus gespeicherten Einstellungen)\n"
"  --password <pass>   RPC-Passwort (Standard: aus gespeicherten Einstellungen)\n"
"  -h, --help          Zeigt diese Meldung an\n"
"\n"
"Standardmäßig werden Host/Port/Benutzer/Passwort aus der gespeicherten\n"
"Einstellungsdatei gelesen (~/.config/tv-transmission/settings.json),\n"
"derselben, die im Einstellungsfenster der TUI festgelegt wird — sie\n"
"müssen also nicht bei jedem Befehl erneut angegeben werden. Jede der\n"
"Optionen --host/--port/--user/--password überschreibt nur diesen einen Wert.";
                case Language::Spanish: return
"Uso: tv-transmission [opciones globales] <comando> [argumentos]\n"
"\n"
"Sin argumentos, inicia la TUI interactiva.\n"
"\n"
"Comandos:\n"
"  list                        Lista todos los torrents\n"
"  add <magnet|url|ruta>       Añade un nuevo torrent\n"
"  start <id>                  Inicia (reanuda) un torrent\n"
"  stop <id>                   Detiene (pausa) un torrent\n"
"  remove <id> [--delete-data] Elimina un torrent (opcionalmente borrando los datos locales)\n"
"  help                        Muestra este mensaje\n"
"\n"
"Opciones globales:\n"
"  --host <host>       Host RPC de Transmission (por defecto: ajustes guardados)\n"
"  --port <puerto>     Puerto RPC de Transmission (por defecto: ajustes guardados)\n"
"  --user <usuario>    Usuario RPC (por defecto: ajustes guardados)\n"
"  --password <clave>  Contraseña RPC (por defecto: ajustes guardados)\n"
"  -h, --help          Muestra este mensaje\n"
"\n"
"Por defecto host/puerto/usuario/contraseña se leen del archivo de\n"
"ajustes guardados (~/.config/tv-transmission/settings.json), el mismo\n"
"que se configura desde la ventana Ajustes de la TUI, por lo que no hace\n"
"falta pasarlos en cada comando. Cada una de las opciones\n"
"--host/--port/--user/--password sobrescribe solo ese valor.";
                default: return
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
            }
            return ""; // unreachable (every Language enumerator handled above)

        case Str::CliErrorMissingArgument:
            return pick("Missing argument: %s", "Argomento mancante: %s", "Argument manquant : %s",
                        "Fehlendes Argument: %s", "Falta el argumento: %s");
        case Str::CliErrorUnknownCommand:
            return pick("Unknown command: %s", "Comando sconosciuto: %s", "Commande inconnue : %s",
                        "Unbekannter Befehl: %s", "Comando desconocido: %s");
        case Str::CliErrorInvalidId:
            return pick("Invalid torrent id: %s", "ID torrent non valido: %s", "Identifiant de torrent invalide : %s",
                        "Ungültige Torrent-ID: %s", "ID de torrent no válido: %s");
        case Str::CliListEmpty:
            return pick("No torrents.", "Nessun torrent.", "Aucun torrent.", "Keine Torrents.", "Sin torrents.");
        case Str::CliAddSuccess:
            return pick("Torrent added.", "Torrent aggiunto.", "Torrent ajouté.", "Torrent hinzugefügt.", "Torrent añadido.");
        case Str::CliAddFailure:
            return pick("Failed to add torrent.", "Aggiunta del torrent non riuscita.", "Échec de l'ajout du torrent.",
                        "Hinzufügen des Torrents fehlgeschlagen.", "No se pudo añadir el torrent.");
        case Str::CliAddDuplicate:
            return pick("Torrent was already present.", "Il torrent era già presente.", "Le torrent était déjà présent.",
                        "Der Torrent war bereits vorhanden.", "El torrent ya estaba presente.");
        case Str::CliStartSuccess:
            return pick("Torrent started.", "Torrent avviato.", "Torrent démarré.", "Torrent gestartet.", "Torrent iniciado.");
        case Str::CliStartFailure:
            return pick("Failed to start torrent.", "Avvio del torrent non riuscito.", "Échec du démarrage du torrent.",
                        "Starten des Torrents fehlgeschlagen.", "No se pudo iniciar el torrent.");
        case Str::CliStopSuccess:
            return pick("Torrent stopped.", "Torrent fermato.", "Torrent arrêté.", "Torrent gestoppt.", "Torrent detenido.");
        case Str::CliStopFailure:
            return pick("Failed to stop torrent.", "Arresto del torrent non riuscito.", "Échec de l'arrêt du torrent.",
                        "Stoppen des Torrents fehlgeschlagen.", "No se pudo detener el torrent.");
        case Str::CliRemoveSuccess:
            return pick("Torrent removed.", "Torrent rimosso.", "Torrent supprimé.", "Torrent entfernt.", "Torrent eliminado.");
        case Str::CliRemoveFailure:
            return pick("Failed to remove torrent.", "Rimozione del torrent non riuscita.", "Échec de la suppression du torrent.",
                        "Entfernen des Torrents fehlgeschlagen.", "No se pudo eliminar el torrent.");
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
