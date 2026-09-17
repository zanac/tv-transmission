#include "Strings.h"

namespace {
Language g_language = Language::English;

// Seven-way pick instead of the old English/Italian ternary — every
// call site below passes (english, italian, french, german, spanish,
// portuguese-brazilian, russian) in that fixed order, which the case
// list groups by column for the same reason the old ternary style did:
// an unhandled string here is a mistranslation, not a build error, so
// keeping seven aligned columns makes a missed/misordered language
// visually obvious on review.
const char* pick(const char* en, const char* it, const char* fr, const char* de, const char* es,
                  const char* pt, const char* ru) {
    switch (g_language) {
        case Language::Italian: return it;
        case Language::French:  return fr;
        case Language::German:  return de;
        case Language::Spanish: return es;
        case Language::PortugueseBrazilian: return pt;
        case Language::Russian: return ru;
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
        case Str::MenuAdd:       return pick("~A~dd...", "~A~ggiungi...", "~A~jouter...", "~H~inzufügen...", "~A~ñadir...", "~A~dicionar...", "~Д~обавить...");
        case Str::MenuStart:     return pick("~S~tart", "~S~tart", "~D~émarrer", "~S~tarten", "~I~niciar", "~I~niciar", "~З~апустить");
        case Str::MenuStop:      return pick("S~t~op", "S~t~op", "Arrê~t~er", "S~t~oppen", "~D~etener", "~P~arar", "~О~становить");
        case Str::MenuRemove:    return pick("~R~emove", "~R~imuovi", "~S~upprimer", "~E~ntfernen", "~E~liminar", "~R~emover", "~У~далить");
        case Str::MenuConnection:
            return pick("~C~onnection...", "~C~onnessione...", "C~o~nnexion...",
                        "~V~erbindung...", "C~o~nexión...", "~C~onexão...", "Под~к~лючение...");
        case Str::MenuServerSettings:
            return pick("~S~erver...", "~S~erver...", "~S~erveur...", "~S~erver...", "~S~ervidor...", "~S~ervidor...", "~С~ервер...");
        case Str::MenuSessionStats:
            return pick("Session ~S~tatistics...", "~S~tatistiche sessione...", "~S~tatistiques de session...",
                        "Sitzungs~s~tatistik...", "E~s~tadísticas de sesión...", "Estatísticas da ~S~essão...", "~С~татистика сессии...");
        case Str::MenuQuit:      return pick("~Q~uit", "~E~sci", "~Q~uitter", "~B~eenden", "~S~alir", "~S~air", "~В~ыход");
        case Str::MenuSettingsMenu: return pick("~S~ettings", "~I~mpostazioni", "~P~aramètres", "~E~instellungen", "~C~onfiguración", "~C~onfigurações", "~Н~астройки");
        case Str::MenuColumnsMenu:
            return pick("~C~olumns", "~C~olonne", "~C~olonnes", "~S~palten", "Co~l~umnas", "~C~olunas", "Ст~о~лбцы");
        case Str::MenuFilters:      return pick("~F~ilters...", "~F~iltri...", "~F~iltres...", "~F~ilter...", "~F~iltros...", "~F~iltros...", "~Ф~ильтры...");
        case Str::MenuConnectionsMenu:
            return pick("Con~n~ections", "Conn~e~ssioni", "Co~n~nexions", "~V~erbindungen", "Co~n~exiones", "Cone~x~ões", "Под~к~лючения");
        case Str::MenuConnectionsEmpty:
            return pick("Empty", "Vuoto", "Vide", "Leer", "Vacío", "Vazio", "Пусто");
        case Str::MenuManageColumns:
            return pick("~M~anage columns...", "~G~estisci colonne...", "~G~érer les colonnes...",
                        "Spalten ~v~erwalten...", "~G~estionar columnas...", "~G~erenciar colunas...", "~У~правление столбцами...");
        case Str::DialogTitleFilters:
            return pick("Filters", "Filtri", "Filtres", "Filter", "Filtros", "Filtros", "Фильтры");
        case Str::LabelFilterName:
            return pick("Name contains:", "Il nome contiene:", "Le nom contient :",
                        "Name enthält:", "El nombre contiene:", "O nome contém:", "Имя содержит:");
        case Str::LabelFilterStatusSection:
            return pick("Show status:", "Mostra stato:", "Afficher l'état :",
                        "Status anzeigen:", "Mostrar estado:", "Mostrar status:", "Показывать статус:");
        case Str::ButtonReset:
            return pick("Reset", "Ripristina", "Réinitialiser", "Zurücksetzen", "Restablecer", "Redefinir", "Сбросить");
        case Str::MsgLanguageChangeRestart:
            return pick(
                "Language changed. Restart the application for the menu bar and "
                "status bar to fully switch over (everything else already has).",
                "Lingua cambiata. Riavvia l'applicazione perché il menu e la barra "
                "di stato si aggiornino del tutto (il resto è già cambiato).",
                "Langue changée. Redémarrez l'application pour que la barre de "
                "menus et la barre d'état basculent complètement (le reste a déjà changé).",
                "Sprache geändert. Starten Sie die Anwendung neu, damit Menüleiste "
                "und Statusleiste vollständig umschalten (alles andere hat sich bereits geändert).",
                "Idioma cambiado. Reinicia la aplicación para que la barra de menús "
                "y la barra de estado cambien del todo (el resto ya ha cambiado).", "Idioma alterado. Reinicie o aplicativo para que a barra de menus e a barra de status mudem completamente (o resto já mudou).", "Язык изменён. Перезапустите приложение, чтобы строка меню и строка состояния переключились полностью (всё остальное уже изменено).");
        case Str::MsgServerAdded:
            return pick("Server '%s' added.", "Server '%s' aggiunto.", "Serveur « %s » ajouté.",
                        "Server '%s' hinzugefügt.", "Servidor '%s' añadido.", "Servidor '%s' adicionado.", "Сервер '%s' добавлен.");
        case Str::MsgServerRemoved:
            return pick("Server '%s' removed.", "Server '%s' rimosso.", "Serveur « %s » supprimé.",
                        "Server '%s' entfernt.", "Servidor '%s' eliminado.", "Servidor '%s' removido.", "Сервер '%s' удалён.");
        case Str::MsgConnectionTestFailed:
            return pick("Could not connect: %s", "Impossibile connettersi: %s", "Connexion impossible : %s",
                        "Verbindung fehlgeschlagen: %s", "No se pudo conectar: %s", "Não foi possível conectar: %s", "Не удалось подключиться: %s");
        case Str::DialogTitleColumnManager:
            return pick("Manage columns", "Gestisci colonne", "Gérer les colonnes",
                        "Spalten verwalten", "Gestionar columnas", "Gerenciar colunas", "Управление столбцами");
        case Str::LabelColumnManagerColumn:
            return pick("Column", "Colonna", "Colonne", "Spalte", "Columna", "Coluna", "Столбец");
        case Str::LabelColumnManagerWidth:
            return pick("Width", "Larghezza", "Largeur", "Breite", "Ancho", "Largura", "Ширина");
        case Str::LabelColumnManagerVisible:
            return pick("Visible", "Visibile", "Visible", "Sichtbar", "Visible", "Visível", "Видимый");
        case Str::ButtonResizeColumn:
            return pick("~R~esize", "~R~idimensiona", "~R~edimensionner", "~G~röße ändern", "~R~edimensionar", "~R~edimensionar", "~И~зменить размер");
        case Str::ButtonMoveColumn:
            return pick("~M~ove", "~S~posta", "Dé~p~lacer", "~V~erschieben", "~M~over", "~M~over", "~П~ереместить");
        case Str::ButtonToggleVisible:
            return pick("~T~oggle visible", "~M~ostra/Nascondi", "~A~fficher/Masquer",
                        "S~i~chtbarkeit", "~O~cultar/Mostrar", "~M~ostrar/Ocultar", "~П~оказать/скрыть");
        case Str::MenuVerify:      return pick("~V~erify", "~V~erifica", "~V~érifier", "~P~rüfen", "~V~erificar", "~V~erificar", "~П~роверить");
        case Str::MenuReannounce:  return pick("Reannoun~c~e", "Ricontatta tra~c~ker", "Réannon~c~er", "Tracker neu ~a~nfragen", "Reanun~c~iar", "Reanun~c~iar", "Пере~о~бъявить");
        case Str::MenuStartNow:    return pick("Start ~N~ow", "Avvia s~u~bito", "Démarrer ~m~aintenant", "~J~etzt starten", "Iniciar ~a~hora", "Iniciar ~a~gora", "Запустить ~с~ейчас");
        case Str::MenuShowDetails: return pick("~D~etails", "~D~ettagli", "Détai~l~s", "~D~etails", "Detal~l~es", "~D~etalhes", "~П~одробности");
        case Str::MenuShowFiles:   return pick("~F~iles", "Fi~l~e", "~F~ichiers", "Date~i~en", "A~r~chivos", "~A~rquivos", "~Ф~айлы");
        case Str::MenuSelectMultiple:
            return pick("Select ~M~ultiple", "Selezione ~m~ultipla", "Sélection ~m~ultiple",
                        "~M~ehrfachauswahl", "Selección ~m~últiple", "Seleção ~m~últipla", "~М~ножественный выбор");
        case Str::MenuCancelSelection:
            return pick("~C~ancel selection", "~A~nnulla selezione", "~A~nnuler la sélection",
                        "Auswahl ~a~bbrechen", "~C~ancelar selección", "~C~ancelar seleção", "~О~тменить выбор");
        case Str::MenuQueue:
            return pick("Q~u~eue", "C~o~da", "F~i~le d'attente", "~W~arteschlange", "C~o~la", "F~i~la", "О~ч~ередь");
        case Str::MenuQueueMoveTop:
            return pick("Move to ~T~op", "Porta in ~c~ima", "Déplacer tout en ~h~aut",
                        "~G~anz nach oben", "Mover al ~p~rincipio", "Mover para o ~t~opo", "Переместить нав~е~рх");
        case Str::MenuQueueMoveUp:
            return pick("Move ~U~p", "Sposta ~s~u", "~M~onter",
                        "Nach ~o~ben", "Mover ~a~rriba", "Mover para ~c~ima", "Переместить в~в~ерх");
        case Str::MenuQueueMoveDown:
            return pick("Move ~D~own", "Sposta ~g~iù", "~D~escendre",
                        "Nach ~u~nten", "Mover a~b~ajo", "Mover para ~b~aixo", "Переместить в~н~из");
        case Str::MenuQueueMoveBottom:
            return pick("Move to ~B~ottom", "Porta in ~f~ondo", "Déplacer tout en ~b~as",
                        "Ganz nach u~n~ten", "Mover al ~f~inal", "Mover para o ~f~im", "Переместить в конец");
        case Str::MenuPriority:
            return pick("Priorit~y~", "Priorit~à~", "Priorit~é~", "P~r~iorität", "Prioridad", "Priorida~d~e", "П~р~иоритет");
        case Str::MenuPriorityLow:
            return pick("~L~ow", "~B~assa", "~B~asse", "~N~iedrig", "~B~aja", "~B~aixa", "~Н~изкий");
        case Str::MenuPriorityNormal:
            return pick("~N~ormal", "~N~ormale", "~N~ormale", "~N~ormal", "~N~ormal", "~N~ormal", "~О~бычный");
        case Str::MenuPriorityHigh:
            return pick("~H~igh", "~A~lta", "~H~aute", "~H~och", "~A~lta", "~A~lta", "~В~ысокий");
        case Str::MenuDeleteWithData:
            return pick("Delete (~w~ith files)", "Elimina (con ~f~ile)", "~E~ffacer (avec fichiers)",
                        "~L~öschen (mit Dateien)", "~B~orrar (con archivos)", "Excluir (com ~a~rquivos)", "Удалить (с ~ф~айлами)");
        case Str::ConfirmRemoveTorrent:
            return pick("Remove '%s' from the list? Files on disk will be kept.",
                        "Rimuovere '%s' dalla lista? I file su disco resteranno.",
                        "Retirer '%s' de la liste ? Les fichiers sur le disque seront conservés.",
                        "'%s' aus der Liste entfernen? Die Dateien auf der Festplatte bleiben erhalten.",
                        "¿Quitar '%s' de la lista? Los archivos en el disco se conservarán.", "Remover '%s' da lista? Os arquivos no disco serão mantidos.", "Удалить '%s' из списка? Файлы на диске будут сохранены.");
        case Str::ConfirmDeleteTorrentWithData:
            return pick("Delete '%s' AND its files on disk? This cannot be undone.",
                        "Eliminare '%s' E i suoi file su disco? L'operazione non si puo' annullare.",
                        "Supprimer '%s' ET ses fichiers sur le disque ? Cette action est irréversible.",
                        "'%s' UND die zugehörigen Dateien löschen? Dies kann nicht rückgängig gemacht werden.",
                        "¿Eliminar '%s' Y sus archivos en el disco? Esta acción no se puede deshacer.", "Excluir '%s' E seus arquivos no disco? Esta ação não pode ser desfeita.", "Удалить '%s' И его файлы на диске? Это действие нельзя отменить.");
        case Str::ConfirmRemoveTorrentsMulti:
            return pick("Remove %s torrents from the list? Files on disk will be kept.",
                        "Rimuovere %s torrent dalla lista? I file su disco resteranno.",
                        "Retirer %s torrents de la liste ? Les fichiers sur le disque seront conservés.",
                        "%s Torrents aus der Liste entfernen? Die Dateien auf der Festplatte bleiben erhalten.",
                        "¿Quitar %s torrents de la lista? Los archivos en el disco se conservarán.", "Remover %s torrents da lista? Os arquivos no disco serão mantidos.", "Удалить %s торрентов из списка? Файлы на диске будут сохранены.");
        case Str::ConfirmDeleteTorrentsWithDataMulti:
            return pick("Delete %s torrents AND their files on disk? This cannot be undone.",
                        "Eliminare %s torrent E i loro file su disco? L'operazione non si puo' annullare.",
                        "Supprimer %s torrents ET leurs fichiers sur le disque ? Cette action est irréversible.",
                        "%s Torrents UND ihre Dateien löschen? Dies kann nicht rückgängig gemacht werden.",
                        "¿Eliminar %s torrents Y sus archivos en el disco? Esta acción no se puede deshacer.", "Excluir %s torrents E seus arquivos no disco? Esta ação não pode ser desfeita.", "Удалить %s торрентов И их файлы на диске? Это действие нельзя отменить.");

        // Standard tvision window-management menu: we just label items
        // that send tvision's own standard commands (cmZoom, cmNext,
        // cmClose, cmTile, cmCascade) — the logic already lives in
        // tvision itself (TWindow/TDeskTop/TApplication).
        case Str::MenuWindow:        return pick("~W~indow", "~F~inestra", "~F~enêtre", "~F~enster", "~V~entana", "~J~anela", "~О~кно");
        case Str::MenuWindowZoom:    return "~Z~oom"; // same word in all five languages
        case Str::MenuWindowNext:    return pick("~N~ext", "~S~uccessiva", "~S~uivante", "~N~ächstes", "~S~iguiente", "Pró~x~ima", "~С~ледующее");
        case Str::MenuWindowClose:   return pick("~C~lose", "~C~hiudi", "~F~ermer", "~S~chließen", "~C~errar", "Fe~c~har", "~З~акрыть");
        case Str::MenuWindowTile:    return pick("~T~ile", "~R~iquadra", "~M~osaïque", "~K~acheln", "~M~osaico", "~L~adrilho", "~М~озаика");
        case Str::MenuWindowCascade: return pick("C~a~scade", "Casc~a~ta", "Casc~a~de", "Kas~a~de", "Casc~a~da", "Casc~a~ta", "Кас~к~ад");
        case Str::MenuWindowList:    return pick("Window ~l~ist", "~E~lenco finestre", "~L~iste des fenêtres",
                                                  "~F~ensterliste", "~L~ista de ventanas", "~L~ista de janelas", "~С~писок окон");
        // Separate from MenuWindowList on purpose: menu/status labels use
        // ~x~ markup to underline a hotkey letter, which only TMenuItem/
        // TStatusItem/TButton interpret. A TDialog title does NOT strip
        // it, so reusing the menu label here would show literal tildes
        // in the title bar.
        case Str::DialogTitleWindowList:
            return pick("Window list", "Elenco finestre", "Liste des fenêtres", "Fensterliste", "Lista de ventanas", "Lista de janelas", "Список окон");

        case Str::MenuHelp:  return pick("~H~elp", "~A~iuto", "~A~ide", "~H~ilfe", "~A~yuda", "~A~juda", "~С~правка");
        case Str::MenuAbout: return pick("~A~bout...", "~I~nfo su...", "À ~p~ropos...", "Ü~b~er...", "~A~cerca de...", "~S~obre...", "~О~ программе...");
        case Str::DialogTitleAbout:
            return pick("About", "Info su", "À propos", "Über", "Acerca de", "Sobre", "О программе");
        case Str::LabelAboutVersion:
            return pick("Version: %s", "Versione: %s", "Version : %s", "Version: %s", "Versión: %s", "Versão: %s", "Версия: %s");
        case Str::LabelAboutCopyright:
            return "Copyright © %d %s"; // same convention in all five languages

        case Str::StatusAdd:      return pick("~F2~ Add", "~F2~ Aggiungi", "~F2~ Ajouter", "~F2~ Hinzufügen", "~F2~ Añadir", "~F2~ Adicionar", "~F2~ Добавить");
        case Str::StatusStart:    return "~F5~ Start"; // same word in all five languages
        case Str::StatusStop:     return pick("~F6~ Stop", "~F6~ Stop", "~F6~ Arrêter", "~F6~ Stopp", "~F6~ Detener", "~F6~ Parar", "~F6~ Стоп");
        case Str::StatusSettings: return pick("~F9~ Connection", "~F9~ Connessione", "~F9~ Connexion", "~F9~ Verbindung", "~F9~ Conexión", "~F9~ Conexão", "~F9~ Соединение");
        case Str::StatusQuit:     return pick("~Alt-X~ Quit", "~Alt-X~ Esci", "~Alt-X~ Quitter", "~Alt-X~ Beenden", "~Alt-X~ Salir", "~Alt-X~ Sair", "~Alt-X~ Выход");

        case Str::WindowTitleTorrentList:
            return pick("Torrents", "Torrent", "Torrents", "Torrents", "Torrents", "Torrents", "Торренты");
        case Str::WindowTitleOffline:
            return pick("(offline)", "(offline)", "(hors ligne)", "(offline)", "(sin conexión)", "(offline)", "(офлайн)");

        case Str::DialogTitleAddTorrent:
            return pick("Add torrent", "Aggiungi torrent", "Ajouter un torrent", "Torrent hinzufügen", "Añadir torrent", "Adicionar torrent", "Добавить торрент");
        case Str::LabelAddTorrentUrl:
            return pick("Magnet link, .torrent URL or local path:",
                        "Magnet link, URL .torrent o path locale:",
                        "Lien magnet, URL .torrent ou chemin local :",
                        "Magnet-Link, .torrent-URL oder lokaler Pfad:",
                        "Enlace magnet, URL .torrent o ruta local:", "Link magnet, URL .torrent ou caminho local:", "Magnet-ссылка, URL .torrent или локальный путь:");
        case Str::LabelFolderPath:
            return pick("Path:", "Percorso:", "Chemin :", "Pfad:", "Ruta:", "Caminho:", "Путь:");
        case Str::MsgFolderUnreadable:
            return pick("(cannot read this directory)", "(impossibile leggere questa cartella)",
                        "(impossible de lire ce dossier)", "(Ordner kann nicht gelesen werden)",
                        "(no se puede leer esta carpeta)", "(não é possível ler esta pasta)", "(не удаётся прочитать эту папку)");
        case Str::MsgTorrentDuplicate:
            return pick("This torrent was already in the list.",
                        "Questo torrent era già presente nella lista.",
                        "Ce torrent était déjà dans la liste.",
                        "Dieser Torrent war bereits in der Liste.",
                        "Este torrent ya estaba en la lista.", "Este torrent já estava na lista.", "Этот торрент уже был в списке.");
        case Str::MsgTorrentAddFailed:
            return pick("Failed to add the torrent:\n%s",
                        "Aggiunta del torrent non riuscita:\n%s",
                        "Échec de l'ajout du torrent :\n%s",
                        "Hinzufügen des Torrents fehlgeschlagen:\n%s",
                        "No se pudo añadir el torrent:\n%s", "Falha ao adicionar o torrent:\n%s", "Не удалось добавить торрент:\n%s");
        case Str::ButtonRename:
            return pick("Rename", "Rinomina", "Renommer", "Umbenennen", "Renombrar", "Renomear", "Переименовать");
        case Str::DialogTitleRename:
            return pick("Rename", "Rinomina", "Renommer", "Umbenennen", "Renombrar", "Renomear", "Переименовать");
        case Str::LabelRenameNewName:
            return pick("New name:", "Nuovo nome:", "Nouveau nom :", "Neuer Name:", "Nuevo nombre:", "Novo nome:", "Новое имя:");
        case Str::MsgRenameFailed:
            return pick("Failed to rename:\n%s",
                        "Rinomina non riuscita:\n%s",
                        "Échec du renommage :\n%s",
                        "Umbenennen fehlgeschlagen:\n%s",
                        "No se pudo renombrar:\n%s", "Falha ao renomear:\n%s", "Не удалось переименовать:\n%s");

        case Str::ButtonOK:     return "OK"; // same word in all five languages
        case Str::ButtonCancel: return pick("Cancel", "Annulla", "Annuler", "Abbrechen", "Cancelar", "Cancelar", "Отмена");
        case Str::ButtonSave: return pick("Save", "Salva", "Enregistrer", "Speichern", "Guardar", "Salvar", "Сохранить");
        case Str::ButtonSelect: return pick("Select", "Seleziona", "Sélectionner", "Auswählen", "Seleccionar", "Selecionar", "Выбрать");

        case Str::DialogTitleConnection:
            return pick("Connection", "Connessione", "Connexion", "Verbindung", "Conexión", "Conexão", "Соединение");
        case Str::DialogTitleServerSettings:
            return pick("Server Configuration", "Configurazione server", "Configuration du serveur",
                        "Server-Konfiguration", "Configuración del servidor", "Configuração do servidor", "Настройки сервера");
        case Str::DialogTitleSessionStats:
            return pick("Session Statistics", "Statistiche sessione", "Statistiques de session",
                        "Sitzungsstatistik", "Estadísticas de sesión", "Estatísticas da sessão", "Статистика сессии");
        case Str::DialogTitleSelectFolder:
            return pick("Select Folder", "Seleziona cartella", "Sélectionner un dossier",
                        "Ordner auswählen", "Seleccionar carpeta", "Selecionar pasta", "Выбрать папку");
        case Str::LabelRefreshSeconds:
            return pick("Refresh (seconds):", "Refresh (secondi):", "Actualisation (secondes) :",
                        "Aktualisierung (Sekunden):", "Actualización (segundos):", "Atualização (segundos):", "Обновление (секунды):");
        case Str::LabelHost:
            return pick("Transmission host:", "Host Transmission:", "Hôte Transmission :",
                        "Transmission-Host:", "Host de Transmission:", "Host do Transmission:", "Хост Transmission:");
        case Str::LabelPort:
            return pick("RPC port:", "Porta RPC:", "Port RPC :", "RPC-Port:", "Puerto RPC:", "Porta RPC:", "Порт RPC:");
        case Str::LabelRpcPath:
            return pick("RPC path:", "Percorso RPC:", "Chemin RPC :", "RPC-Pfad:", "Ruta RPC:", "Caminho RPC:", "Путь RPC:");
        case Str::LabelUser:
            return pick("User (optional):", "Utente (opzionale):", "Utilisateur (facultatif) :",
                        "Benutzer (optional):", "Usuario (opcional):", "Usuário (opcional):", "Пользователь (необязательно):");
        case Str::LabelPassword:
            return pick("Password (optional):", "Password (opzionale):", "Mot de passe (facultatif) :",
                        "Passwort (optional):", "Contraseña (opcional):", "Senha (opcional):", "Пароль (необязательно):");
        case Str::LabelLanguage:
            return pick("Language:", "Lingua:", "Langue :", "Sprache:", "Idioma:", "Idioma:", "Язык:");
        case Str::LabelServerName:
            return pick("Server name:", "Nome server:", "Nom du serveur :",
                        "Servername:", "Nombre del servidor:", "Nome do servidor:", "Имя сервера:");
        case Str::DefaultServerName:
            return pick("server", "server", "serveur", "Server", "servidor", "servidor", "сервер");
        // Native language names: identical regardless of the currently
        // selected language, so they stay recognizable to someone
        // looking for their own language in the list.
        case Str::LanguageEnglish: return "English";
        case Str::LanguageItalian: return "Italiano";
        case Str::LanguageFrench:  return "Français";
        case Str::LanguageGerman:  return "Deutsch";
        case Str::LanguageSpanish: return "Español";
        case Str::LanguagePortugueseBrazilian: return "Português (Brasil)";
        case Str::LanguageRussian: return "Русский";

        case Str::WindowTitleDetails:
            return pick("Torrent details", "Dettagli torrent", "Détails du torrent", "Torrent-Details", "Detalles del torrent", "Detalhes do torrent", "Сведения о торренте");
        case Str::LabelName:
            return pick("Name: %s", "Nome: %s", "Nom : %s", "Name: %s", "Nombre: %s", "Nome: %s", "Имя: %s");
        case Str::LabelSize:
            return pick("Size: %s", "Dimensione: %s", "Taille : %s", "Größe: %s", "Tamaño: %s", "Tamanho: %s", "Размер: %s");
        case Str::LabelCompleted:
            return pick("Completed: %.1f%%", "Completato: %.1f%%", "Terminé : %.1f%%",
                        "Abgeschlossen: %.1f%%", "Completado: %.1f%%", "Concluído: %.1f%%", "Завершено: %.1f%%");
        case Str::LabelDownload:  return "Download: %.1f KB/s"; // "Download" is the same word in all five here
        case Str::LabelUpload:    return "Upload: %.1f KB/s";   // same reasoning
        case Str::LabelStatus:
            return pick("Status: %s", "Stato: %s", "État : %s", "Status: %s", "Estado: %s", "Status: %s", "Статус: %s");
        case Str::LabelError:
            return pick("Error: %s", "Errore: %s", "Erreur : %s", "Fehler: %s", "Error: %s", "Erro: %s", "Ошибка: %s");
        case Str::LabelId:        return "ID: %d"; // same in all five
        case Str::LabelAdded:
            return pick("Added: %s", "Aggiunto: %s", "Ajouté : %s", "Hinzugefügt: %s", "Añadido: %s", "Adicionado: %s", "Добавлено: %s");

        case Str::LabelLocation:
            return pick("Location: %s", "Posizione: %s", "Emplacement : %s", "Speicherort: %s", "Ubicación: %s", "Local: %s", "Расположение: %s");
        case Str::LabelPrivacyPublic:
            return pick("Privacy: public torrent", "Privacy: torrent pubblico", "Confidentialité : torrent public",
                        "Datenschutz: öffentlicher Torrent", "Privacidad: torrent público", "Privacidade: torrent público", "Конфиденциальность: публичный торрент");
        case Str::LabelPrivacyPrivate:
            return pick("Privacy: private torrent", "Privacy: torrent privato", "Confidentialité : torrent privé",
                        "Datenschutz: privater Torrent", "Privacidad: torrent privado", "Privacidade: torrent privado", "Конфиденциальность: приватный торрент");
        case Str::LabelMagnet:    return "Magnet: %s"; // same word in all five
        case Str::LabelPieces:
            return pick("Pieces: %lld of %s", "Sezioni: %lld da %s", "Morceaux : %lld de %s",
                        "Teile: %lld von %s", "Piezas: %lld de %s", "Partes: %lld de %s", "Части: %lld из %s");

        case Str::LabelAvailable:
            return pick("Available: %.1f%%", "Disponibile: %.1f%%", "Disponible : %.1f%%",
                        "Verfügbar: %.1f%%", "Disponible: %.1f%%", "Disponível: %.1f%%", "Доступно: %.1f%%");
        case Str::LabelDownloadedTotal:
            return pick("Downloaded: %s", "Scaricato: %s", "Téléchargé : %s", "Heruntergeladen: %s", "Descargado: %s", "Baixado: %s", "Скачано: %s");
        case Str::LabelUploadedTotal:
            return pick("Uploaded: %s (Ratio: %.2f)", "Inviato: %s (Ratio: %.2f)", "Envoyé : %s (Ratio : %.2f)",
                        "Hochgeladen: %s (Verhältnis: %.2f)", "Subido: %s (Ratio: %.2f)", "Enviado: %s (Proporção: %.2f)", "Отдано: %s (Рейтинг: %.2f)");
        case Str::LabelAverageSpeed:
            return pick("Average speed: %s", "Velocità media: %s", "Vitesse moyenne : %s",
                        "Durchschnittsgeschwindigkeit: %s", "Velocidad media: %s", "Velocidade média: %s", "Средняя скорость: %s");

        case Str::LabelLastActivity:
            return pick("Last activity: %s", "Ultima attività: %s", "Dernière activité : %s",
                        "Letzte Aktivität: %s", "Última actividad: %s", "Última atividade: %s", "Последняя активность: %s");

        case Str::LabelTimeDownloading:
            return pick("Downloading: %s", "In download: %s", "Téléchargement : %s", "Herunterladen: %s", "Descargando: %s", "Baixando: %s", "Скачивание: %s");
        case Str::LabelTimeSeeding:
            return pick("Seeding: %s", "In seeding: %s", "Partage : %s", "Seeding: %s", "Compartiendo: %s", "Semeando: %s", "Раздача: %s");

        case Str::LabelSpeedLimitSection:
            return pick("Speed limit for this torrent:", "Limite di velocità per questo torrent:",
                        "Limite de vitesse pour ce torrent :", "Geschwindigkeitslimit für diesen Torrent:",
                        "Límite de velocidad para este torrent:", "Limite de velocidade para este torrent:", "Ограничение скорости для этого торрента:");
        case Str::LabelSeedRatioSection:
            return pick("Seed ratio limit for this torrent:", "Limite di rapporto per questo torrent:",
                        "Limite de partage pour ce torrent :", "Verhältnislimit für diesen Torrent:",
                        "Límite de ratio para este torrent:", "Limite de compartilhamento para este torrent:",
                        "Ограничение рейтинга для этого торрента:");
        case Str::RadioSeedRatioGlobal:
            return pick("Use global setting", "Usa impostazione globale", "Utiliser le réglage global",
                        "Globale Einstellung verwenden", "Usar configuración global",
                        "Usar configuração global", "Использовать глобальную настройку");
        case Str::RadioSeedRatioCustom:
            return pick("Stop seeding at ratio:", "Ferma il seed al rapporto:", "Arrêter le partage au ratio :",
                        "Verteilen stoppen bei Verhältnis:", "Detener siembra en la proporción:",
                        "Parar de semear na proporção:", "Остановить раздачу при рейтинге:");
        case Str::RadioSeedRatioUnlimited:
            return pick("Seed indefinitely", "Semina senza limiti", "Partager indéfiniment",
                        "Unbegrenzt verteilen", "Sembrar indefinidamente",
                        "Semear indefinidamente", "Раздавать без ограничений");
        case Str::CheckLimitDownload:
            return pick("Limit download", "Limita download", "Limiter le téléchargement",
                        "Download begrenzen", "Limitar descarga", "Limitar download", "Ограничить скачивание");
        case Str::CheckLimitUpload:
            return pick("Limit upload", "Limita upload", "Limiter l'envoi", "Upload begrenzen", "Limitar subida", "Limitar envio", "Ограничить отдачу");
        case Str::UnitKBs: return "KB/s"; // same in all five
        case Str::CheckHonorGlobalLimits:
            return pick("Honor global speed limits", "Rispetta i limiti globali",
                        "Respecter les limites globales", "Globale Limits berücksichtigen",
                        "Respetar los límites globales", "Respeitar limites globais", "Учитывать глобальные ограничения");
        case Str::ButtonApply: return pick("Apply", "Applica", "Appliquer", "Anwenden", "Aplicar", "Aplicar", "Применить");
        // Distinct from MenuWindowClose on purpose: same reason as
        // DialogTitleWindowList above (menu labels carry ~x~ hotkey
        // markup that a TButton here doesn't need and would show as
        // literal tildes if reused verbatim... actually TButton DOES
        // interpret ~x~, but keeping a separate plain string here avoids
        // any accidental coupling between this window's button and the
        // Window menu's Close item, which apply to different things
        // (this torrent's window vs. "whichever window is active").
        case Str::ButtonClose: return pick("Close", "Chiudi", "Fermer", "Schließen", "Cerrar", "Fechar", "Закрыть");
        case Str::ButtonTrackers: return pick("Trackers...", "Tracker...", "Trackers...", "Tracker...", "Trackers...", "Trackers...", "Трекеры...");
        case Str::ButtonRefresh: return pick("Refresh", "Aggiorna", "Actualiser", "Aktualisieren", "Actualizar", "Atualizar", "Обновить");
        case Str::ButtonBrowse: return pick("Browse...", "Sfoglia...", "Parcourir...", "Durchsuchen...", "Examinar...", "Procurar...", "Обзор...");
        case Str::ButtonChangeFolder: return pick("Change...", "Cambia...", "Changer...", "Ändern...", "Cambiar...", "Alterar...", "Изменить...");
        case Str::ButtonVerify: return pick("Verify", "Verifica", "Vérifier", "Prüfen", "Verificar", "Verificar", "Проверить");
        case Str::TabTrackers: return pick("Trackers", "Tracker", "Trackers", "Trackers", "Trackers", "Trackers", "Трекеры");
        case Str::TabPeers: return pick("Peers", "Peer", "Pairs", "Peers", "Pares", "Peers", "Пиры");
        case Str::DialogTitleBrowseTorrent:
            return pick("Select a .torrent file", "Seleziona un file .torrent",
                        "Sélectionner un fichier .torrent", "Wähle eine .torrent-Datei",
                        "Selecciona un archivo .torrent", "Selecionar um arquivo .torrent", "Выберите файл .torrent");

        case Str::WindowTitleTrackerList:
            return pick("Trackers: %s", "Tracker: %s", "Trackers : %s", "Tracker: %s", "Trackers: %s", "Trackers: %s", "Трекеры: %s");
        case Str::WindowTitleTrackerDetail:
            return pick("Tracker details", "Dettagli tracker", "Détails du tracker", "Tracker-Details", "Detalles del tracker", "Detalhes do tracker", "Сведения о трекере");
        case Str::HeaderTrackerHost: return "Tracker"; // same word in all five
        case Str::HeaderTier:        return "Tier"; // kept as-is in all five (Transmission's own term)
        case Str::HeaderSeeders:     return "Seeders"; // same word in all five
        case Str::HeaderLeechers:    return "Leechers"; // same word in all five
        case Str::HeaderDownloaded:
            return pick("Downloaded", "Scaricati", "Téléchargé", "Heruntergeladen", "Descargado", "Baixado", "Скачано");
        case Str::HeaderTrackerStatus:
            return pick("Status", "Stato", "État", "Status", "Estado", "Status", "Статус");
        case Str::HeaderPeerAddress:
            return pick("Address", "Indirizzo", "Adresse", "Adresse", "Dirección", "Endereço", "Адрес");
        case Str::HeaderPeerClient:
            return pick("Client", "Client", "Client", "Client", "Cliente", "Cliente", "Клиент");
        case Str::HeaderPeerProgress:
            return pick("Progress", "Avanzamento", "Progression", "Fortschritt", "Progreso", "Progresso", "Прогресс");
        case Str::HeaderPeerDown:
            return pick("Down", "Ricezione", "Réception", "Empfang", "Recepción", "Recepção", "Приём");
        case Str::HeaderPeerUp:
            return pick("Up", "Invio", "Envoi", "Sendung", "Envío", "Envio", "Отдача");
        case Str::HeaderPeerFlags:
            return pick("Flags", "Flag", "Indicateurs", "Flags", "Indicadores", "Sinalizadores", "Флаги");
        case Str::TrackerStatusOk: return "OK"; // same in all five
        case Str::TrackerStatusError:
            return pick("Error", "Errore", "Erreur", "Fehler", "Error", "Erro", "Ошибка");
        case Str::ValueNotAvailable: return "N/A"; // same in all five
        case Str::LabelTrackerHost: return "Tracker: %s"; // same word in all five
        case Str::LabelTrackerTier: return "Tier: %d"; // same in all five
        case Str::LabelTrackerSeeders: return "Seeders: %s"; // same in all five
        case Str::LabelTrackerLeechers: return "Leechers: %s"; // same in all five
        case Str::LabelTrackerDownloaded:
            return pick("Downloaded: %s", "Scaricati: %s", "Téléchargé : %s", "Heruntergeladen: %s", "Descargado: %s", "Baixado: %s", "Скачано: %s");
        case Str::LabelTrackerLastAnnounce:
            return pick("Last announce: %s", "Ultimo annuncio: %s", "Dernière annonce : %s",
                        "Letzte Ankündigung: %s", "Último anuncio: %s", "Último anúncio: %s", "Последнее объявление: %s");
        case Str::LabelTrackerNextAnnounce:
            return pick("Next announce: %s", "Prossimo annuncio: %s", "Prochaine annonce : %s",
                        "Nächste Ankündigung: %s", "Próximo anuncio: %s", "Próximo anúncio: %s", "Следующее объявление: %s");
        case Str::LabelTrackerResult:
            return pick("Result: %s", "Risultato: %s", "Résultat : %s", "Ergebnis: %s", "Resultado: %s", "Resultado: %s", "Результат: %s");

        case Str::LabelGlobalSpeedSection:
            return pick("Global speed limit (all torrents):", "Limite di velocità globale (tutti i torrent):",
                        "Limite de vitesse globale (tous les torrents) :",
                        "Globales Geschwindigkeitslimit (alle Torrents):",
                        "Límite de velocidad global (todos los torrents):", "Limite de velocidade global (todos os torrents):", "Глобальное ограничение скорости (все торренты):");
        case Str::CheckGlobalLimitDownload:
            return pick("Limit download", "Limita download", "Limiter le téléchargement",
                        "Download begrenzen", "Limitar descarga", "Limitar download", "Ограничить скачивание");
        case Str::CheckGlobalLimitUpload:
            return pick("Limit upload", "Limita upload", "Limiter l'envoi", "Upload begrenzen", "Limitar subida", "Limitar envio", "Ограничить отдачу");
        case Str::LabelAltSpeedSection:
            return pick("'Speed Limit' mode", "Modalità 'Limite velocità'", "Mode « Limite de vitesse »",
                        "Modus 'Geschwindigkeitslimit'", "Modo 'Límite de velocidad'", "Modo 'Limite de velocidade'", "Режим «Ограничение скорости»");
        case Str::LabelAltSpeedDescription:
            return pick(
                "When enabled, 'Speed Limit' mode overrides\nthe global bandwidth limit",
                "Quando abilitata, la modalità 'Limite velocità'\nannulla la limitazione globale di banda",
                "Lorsqu'il est activé, le mode « Limite de vitesse »\nremplace la limite de bande passante globale",
                "Wenn aktiviert, setzt der Modus 'Geschwindigkeitslimit'\ndas globale Bandbreitenlimit außer Kraft",
                "Cuando está habilitado, el modo 'Límite de velocidad'\nanula el límite de ancho de banda global", "Quando ativado, o modo 'Limite de velocidade'\nsubstitui o limite global de banda", "Если включён, режим «Ограничение скорости»\nзаменяет глобальное ограничение полосы");
        case Str::LabelAltSpeedDownload:
            return pick("Download limit:", "Limite download:", "Limite de téléchargement :",
                        "Download-Limit:", "Límite de descarga:", "Limite de download:", "Ограничение скачивания:");
        case Str::LabelAltSpeedUpload:
            return pick("Upload limit:", "Limite upload:", "Limite d'envoi :",
                        "Upload-Limit:", "Límite de subida:", "Limite de envio:", "Ограничение отдачи:");
        case Str::LabelNetworkSection:
            return pick("Network", "Rete", "Réseau", "Netzwerk", "Red", "Rede", "Сеть");
        case Str::ButtonTestPort:
            return pick("Test port", "Test porta", "Tester le port", "Port testen", "Probar puerto", "Testar porta", "Проверить порт");
        case Str::ButtonUpdateBlocklist:
            return pick("Update blocklist", "Aggiorna blocklist", "MAJ liste noire",
                        "Sperrliste aktualisieren", "Actualizar lista de bloqueo", "Atualizar lista de bloqueio", "Обновить чёрный список");
        case Str::LabelCurrentSession:
            return pick("Current session", "Sessione corrente", "Session en cours",
                        "Aktuelle Sitzung", "Sesión actual", "Sessão atual", "Текущая сессия");
        case Str::LabelAllTime:
            return pick("All time", "Sempre", "Total", "Insgesamt", "Total", "Total", "За всё время");
        case Str::LabelStatsDownloaded:
            return pick("Downloaded:", "Scaricato:", "Téléchargé :", "Heruntergeladen:", "Descargado:", "Baixado:", "Скачано:");
        case Str::LabelStatsUploaded:
            return pick("Uploaded:", "Caricato:", "Envoyé :", "Hochgeladen:", "Subido:", "Enviado:", "Отдано:");
        case Str::LabelStatsActive:
            return pick("Active:", "Attivo:", "Actif :", "Aktiv:", "Activo:", "Ativo:", "Активно:");
        case Str::LabelStatsStarted:
            return pick("Started %d times", "Avviato %d volte", "Démarré %d fois",
                        "%d Mal gestartet", "Iniciado %d veces", "Iniciado %d vezes", "Запущено %d раз");
        case Str::ResultPortOpen:
            return pick("Port: open", "Porta: aperta", "Port : ouvert", "Port: offen", "Puerto: abierto", "Porta: aberta", "Порт: открыт");
        case Str::ResultPortClosed:
            return pick("Port: closed", "Porta: chiusa", "Port : fermé", "Port: geschlossen", "Puerto: cerrado", "Porta: fechada", "Порт: закрыт");
        case Str::ResultPortTestFailed:
            return pick("Port: test failed", "Porta: test fallito", "Port : échec du test",
                        "Port: Test fehlgeschlagen", "Puerto: prueba fallida", "Porta: teste falhou", "Порт: ошибка проверки");
        case Str::ResultBlocklistUpdated:
            return pick("Blocklist: %d rules", "Blocklist: %d regole", "Liste noire : %d règles",
                        "Sperrliste: %d Regeln", "Lista de bloqueo: %d reglas", "Lista de bloqueio: %d regras", "Чёрный список: %d правил");
        case Str::ResultBlocklistFailed:
            return pick("Blocklist: update failed", "Blocklist: aggiornamento fallito",
                        "Liste noire : échec", "Sperrliste: Fehler", "Lista de bloqueo: error", "Lista de bloqueio: falha na atualização", "Чёрный список: ошибка обновления");
        case Str::ResultTesting:
            return pick("Testing...", "Test in corso...", "Test en cours...", "Teste...", "Probando...", "Testando...", "Проверка...");
        case Str::ResultUpdating:
            return pick("Updating...", "Aggiornamento...", "Mise à jour...", "Aktualisiere...", "Actualizando...", "Atualizando...", "Обновление...");
        case Str::CheckAltSpeedEnabled:
            return pick("Enable 'Speed Limit' mode", "Abilita modalità 'Limite velocità'",
                        "Activer le mode « Limite de vitesse »", "Modus 'Geschwindigkeitslimit' aktivieren",
                        "Habilitar modo 'Límite de velocidad'", "Ativar modo 'Limite de velocidade'", "Включить режим «Ограничение скорости»");

        case Str::TorrentStatusStopped:
            return pick("Stopped", "Fermo", "Arrêté", "Gestoppt", "Detenido", "Parado", "Остановлен");
        case Str::TorrentStatusCheckWait:
            return pick("Queued for check", "In attesa di verifica", "En attente de vérification",
                        "Wartet auf Prüfung", "En espera de verificación", "Na fila para verificação", "В очереди на проверку");
        case Str::TorrentStatusChecking:
            return pick("Checking", "Verifica in corso", "Vérification en cours", "Wird geprüft", "Verificando", "Verificando", "Проверка");
        case Str::TorrentStatusDownloadWait:
            return pick("Queued for download", "In attesa di download", "En attente de téléchargement",
                        "Wartet auf Download", "En espera de descarga", "Na fila para download", "В очереди на скачивание");
        case Str::TorrentStatusDownloading:
            return pick("Downloading", "Download in corso", "Téléchargement en cours", "Wird heruntergeladen", "Descargando", "Baixando", "Скачивание");
        case Str::TorrentStatusSeedWait:
            return pick("Queued for seeding", "In attesa di seeding", "En attente de partage",
                        "Wartet auf Seeding", "En espera de compartir", "Na fila para semear", "В очереди на раздачу");
        case Str::TorrentStatusSeeding:
            return pick("Seeding", "Seeding", "Partage", "Seeding", "Compartiendo", "Semeando", "Раздача");
        case Str::TorrentStatusUnknown:
            return pick("Unknown", "Sconosciuto", "Inconnu", "Unbekannt", "Desconocido", "Desconhecido", "Неизвестно");

        // Torrent list column headers. "Download"/"Upload" stay the same
        // in all five languages: it's the same convention Transmission
        // itself uses in its own translated UIs.
        case Str::HeaderName:
            return pick("Name", "Nome", "Nom", "Name", "Nombre", "Nome", "Имя");
        case Str::HeaderDone:
            return pick("Done", "Compl.", "Fait", "Fertig", "Hecho", "Progr.", "Готово");
        case Str::HeaderSize:
            return pick("Size", "Dim.", "Taille", "Größe", "Tamaño", "Tam.", "Разм.");
        case Str::HeaderDownload: return "Download"; // same word in all five
        case Str::HeaderUpload:   return "Upload"; // same word in all five
        case Str::HeaderId:       return "ID"; // same in all five
        case Str::HeaderStatus:
            return pick("Status", "Stato", "État", "Status", "Estado", "Status", "Статус");
        case Str::HeaderAdded:
            return pick("Added", "Aggiunto", "Ajouté", "Hinzugefügt", "Añadido", "Adic.", "Добавл.");
        case Str::HeaderRatio:
            return pick("Ratio", "Rapporto", "Ratio", "Verhältnis", "Ratio", "Proporção", "Рейтинг");
        case Str::HeaderTotalUploaded:
            return pick("Uploaded", "Caricato", "Envoyé", "Hochgeladen", "Subido", "Enviado", "Отдано");
        case Str::HeaderTotalDownloaded:
            return pick("Downloaded", "Scaricato", "Reçu", "Heruntergeladen", "Descargado", "Baixado", "Скачано");
        case Str::HeaderLocation:
            return pick("Location", "Posizione", "Emplacement", "Speicherort", "Ubicación", "Local", "Расположение");
        case Str::HeaderEta:       return "ETA"; // same abbreviation in all five
        case Str::HeaderPeers:
            return pick("Peers", "Peer", "Pairs", "Peers", "Pares", "Peers", "Пиры");
        case Str::HeaderQueuePosition:
            return pick("Queue", "Coda", "File", "Warteschlange", "Cola", "Fila", "Очередь");
        case Str::HeaderPriority:
            return pick("Priority", "Priorità", "Priorité", "Priorität", "Prioridad", "Prioridade", "Приоритет");
        case Str::HeaderCompletedDate:
            return pick("Completed", "Completato", "Terminé", "Abgeschlossen", "Completado", "Concluído", "Завершено");
        case Str::PriorityLow:
            return pick("Low", "Bassa", "Basse", "Niedrig", "Baja", "Baixa", "Низкий");
        case Str::PriorityNormal:
            return pick("Normal", "Normale", "Normale", "Normal", "Normal", "Normal", "Обычный");
        case Str::PriorityHigh:
            return pick("High", "Alta", "Haute", "Hoch", "Alta", "Alta", "Высокий");

        case Str::WindowTitleFiles:
            return pick("Files: %s", "File: %s", "Fichiers : %s", "Dateien: %s", "Archivos: %s", "Arquivos: %s", "Файлы: %s");
        case Str::HeaderFileName:
            return pick("File", "File", "Fichier", "Datei", "Archivo", "Arquivo", "Файл");
        case Str::HeaderFileSize:
            return pick("Size", "Dimensione", "Taille", "Größe", "Tamaño", "Tamanho", "Размер");
        case Str::HeaderFileProgress:
            return pick("Done", "Fatto", "Terminé", "Fertig", "Hecho", "Feito", "Готово");
        case Str::HeaderFileWanted:
            return pick("Wanted", "Richiesto", "Voulu", "Gewünscht", "Deseado", "Desejado", "Нужен");
        case Str::ValueMixed: return pick("Mixed", "Misto", "Mixte", "Gemischt", "Mixto", "Misto", "Смешанно");
        case Str::ButtonToggleWanted:
            return pick("~T~oggle wanted", "~A~ttiva/disattiva", "Basc~u~ler voulu",
                        "~G~ewünscht umschalten", "Alternar de~s~eado", "~A~lternar desejado", "~П~ереключить");
        case Str::ButtonSelectAll:
            return pick("Select ~a~ll", "Seleziona ~t~utto", "~T~out sélectionner",
                        "~A~lle auswählen", "Seleccionar ~t~odo", "Selecionar ~t~udo", "Выбрать ~в~сё");
        case Str::ButtonSelectNone:
            return pick("Select ~n~one", "Deseleziona t~u~tto", "Tout dé~s~électionner",
                        "Alle a~b~wählen", "Deseleccionar to~d~o", "Deseleci~o~nar tudo", "Снять ~в~ыделение");
        case Str::ButtonPriorityLow:
            return pick("Priority ~L~ow", "Priorità ~b~assa", "Priorité ~b~asse",
                        "Priorität ~n~iedrig", "Prioridad ~b~aja", "Prioridade ~b~aixa", "Приоритет ~н~изкий");
        case Str::ButtonPriorityNormal:
            return pick("Priority ~N~ormal", "Priorità n~o~rmale", "Priorité n~o~rmale",
                        "Priorität n~o~rmal", "Prioridad n~o~rmal", "Prioridade n~o~rmal", "Приоритет об~ы~чный");
        case Str::ButtonPriorityHigh:
            return pick("Priority ~H~igh", "Priorità ~a~lta", "Priorité ~h~aute",
                        "Priorität ~h~och", "Prioridad ~a~lta", "Prioridade ~a~lta", "Приоритет выс~о~кий");

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
"  --rpc-path <path>   Percorso RPC di Transmission (default: transmission/rpc)\n"
"  --user <utente>     Utente RPC (default: dalle impostazioni salvate)\n"
"  --password <pass>   Password RPC (default: dalle impostazioni salvate)\n"
"  -h, --help          Mostra questo messaggio\n"
"\n"
"Di default host/porta/utente/password vengono letti dal file delle\n"
"impostazioni salvate (~/.config/tv-transmission/settings.json), lo\n"
"stesso impostato dalla finestra Impostazioni della TUI, quindi non\n"
"serve passarli a ogni comando. Ognuna delle opzioni --host/--port/\n"
"--rpc-path/--user/--password sovrascrive solo quel valore.";
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
"  --rpc-path <chemin> Chemin RPC de Transmission (par défaut : transmission/rpc)\n"
"  --user <utilisateur> Utilisateur RPC (par défaut : paramètres enregistrés)\n"
"  --password <mdp>    Mot de passe RPC (par défaut : paramètres enregistrés)\n"
"  -h, --help          Affiche ce message\n"
"\n"
"Par défaut, hôte/port/utilisateur/mot de passe sont lus depuis le\n"
"fichier de paramètres enregistrés (~/.config/tv-transmission/settings.json),\n"
"le même que celui défini depuis la fenêtre Paramètres de la TUI, donc\n"
"inutile de les repasser à chaque commande. Chacune des options\n"
"--host/--port/--rpc-path/--user/--password ne remplace que cette valeur-là.";
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
"  --rpc-path <pfad>   Transmission-RPC-Pfad (Standard: transmission/rpc)\n"
"  --user <benutzer>   RPC-Benutzername (Standard: aus gespeicherten Einstellungen)\n"
"  --password <pass>   RPC-Passwort (Standard: aus gespeicherten Einstellungen)\n"
"  -h, --help          Zeigt diese Meldung an\n"
"\n"
"Standardmäßig werden Host/Port/Benutzer/Passwort aus der gespeicherten\n"
"Einstellungsdatei gelesen (~/.config/tv-transmission/settings.json),\n"
"derselben, die im Einstellungsfenster der TUI festgelegt wird — sie\n"
"müssen also nicht bei jedem Befehl erneut angegeben werden. Jede der\n"
"Optionen --host/--port/--rpc-path/--user/--password überschreibt nur diesen einen Wert.";
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
"  --rpc-path <ruta>   Ruta RPC de Transmission (por defecto: transmission/rpc)\n"
"  --user <usuario>    Usuario RPC (por defecto: ajustes guardados)\n"
"  --password <clave>  Contraseña RPC (por defecto: ajustes guardados)\n"
"  -h, --help          Muestra este mensaje\n"
"\n"
"Por defecto host/puerto/usuario/contraseña se leen del archivo de\n"
"ajustes guardados (~/.config/tv-transmission/settings.json), el mismo\n"
"que se configura desde la ventana Ajustes de la TUI, por lo que no hace\n"
"falta pasarlos en cada comando. Cada una de las opciones\n"
"--host/--port/--rpc-path/--user/--password sobrescribe solo ese valor.";
                case Language::PortugueseBrazilian: return
"Uso: tv-transmission [opções globais] <comando> [argumentos]\n"
"\n"
"Sem argumentos, inicia a TUI interativa.\n"
"\n"
"Comandos:\n"
"  list                        Lista todos os torrents\n"
"  add <magnet|url|caminho>    Adiciona um novo torrent\n"
"  start <id>                  Inicia (retoma) um torrent\n"
"  stop <id>                   Para (pausa) um torrent\n"
"  remove <id> [--delete-data] Remove um torrent (opcionalmente apagando os dados locais)\n"
"  help                        Mostra esta mensagem\n"
"\n"
"Opções globais:\n"
"  --host <host>        Host RPC do Transmission (padrão: das configurações salvas)\n"
"  --port <porta>       Porta RPC do Transmission (padrão: das configurações salvas)\n"
"  --rpc-path <caminho> Caminho RPC do Transmission (padrão: transmission/rpc)\n"
"  --user <usuário>     Usuário RPC (padrão: das configurações salvas)\n"
"  --password <senha>   Senha RPC (padrão: das configurações salvas)\n"
"  -h, --help           Mostra esta mensagem\n"
"\n"
"Por padrão, host/porta/usuário/senha são lidos do arquivo de\n"
"configurações salvas (~/.config/tv-transmission/settings.json), o\n"
"mesmo definido pela janela de Configurações da TUI, então não é\n"
"preciso passá-los em cada comando. Qualquer uma das opções\n"
"--host/--port/--rpc-path/--user/--password sobrescreve apenas esse valor.";
                case Language::Russian: return
"Использование: tv-transmission [глобальные параметры] <команда> [аргументы]\n"
"\n"
"Без аргументов запускается интерактивный TUI.\n"
"\n"
"Команды:\n"
"  list                        Показать список всех торрентов\n"
"  add <magnet|url|путь>       Добавить новый торрент\n"
"  start <id>                  Запустить (возобновить) торрент\n"
"  stop <id>                   Остановить (приостановить) торрент\n"
"  remove <id> [--delete-data] Удалить торрент (при желании вместе с локальными данными)\n"
"  help                        Показать это сообщение\n"
"\n"
"Глобальные параметры:\n"
"  --host <хост>         RPC-хост Transmission (по умолчанию: из сохранённых настроек)\n"
"  --port <порт>         RPC-порт Transmission (по умолчанию: из сохранённых настроек)\n"
"  --rpc-path <путь>     RPC-путь Transmission (по умолчанию: transmission/rpc)\n"
"  --user <пользователь> Имя пользователя RPC (по умолчанию: из сохранённых настроек)\n"
"  --password <пароль>   Пароль RPC (по умолчанию: из сохранённых настроек)\n"
"  -h, --help            Показать это сообщение\n"
"\n"
"По умолчанию хост/порт/пользователь/пароль читаются из файла\n"
"сохранённых настроек (~/.config/tv-transmission/settings.json),\n"
"того же, что задаётся в окне настроек TUI, так что не нужно\n"
"передавать их при каждой команде. Любой из параметров\n"
"--host/--port/--rpc-path/--user/--password переопределяет только это значение.";
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
"  --rpc-path <path>   Transmission RPC path (default: transmission/rpc)\n"
"  --user <user>       RPC username (default: from saved settings)\n"
"  --password <pass>   RPC password (default: from saved settings)\n"
"  -h, --help          Show this message\n"
"\n"
"By default host/port/user/password are read from the saved settings\n"
"file (~/.config/tv-transmission/settings.json), the same one set from\n"
"the TUI's Settings window, so you don't need to pass them on every\n"
"command. Any of --host/--port/--rpc-path/--user/--password overrides just that\n"
"one value.";
            }
            return ""; // unreachable (every Language enumerator handled above)

        case Str::CliErrorMissingArgument:
            return pick("Missing argument: %s", "Argomento mancante: %s", "Argument manquant : %s",
                        "Fehlendes Argument: %s", "Falta el argumento: %s", "Argumento ausente: %s", "Отсутствует аргумент: %s");
        case Str::CliErrorUnknownCommand:
            return pick("Unknown command: %s", "Comando sconosciuto: %s", "Commande inconnue : %s",
                        "Unbekannter Befehl: %s", "Comando desconocido: %s", "Comando desconhecido: %s", "Неизвестная команда: %s");
        case Str::CliErrorInvalidId:
            return pick("Invalid torrent id: %s", "ID torrent non valido: %s", "Identifiant de torrent invalide : %s",
                        "Ungültige Torrent-ID: %s", "ID de torrent no válido: %s", "ID de torrent inválido: %s", "Неверный ID торрента: %s");
        case Str::CliListEmpty:
            return pick("No torrents.", "Nessun torrent.", "Aucun torrent.", "Keine Torrents.", "Sin torrents.", "Nenhum torrent.", "Нет торрентов.");
        case Str::CliAddSuccess:
            return pick("Torrent added.", "Torrent aggiunto.", "Torrent ajouté.", "Torrent hinzugefügt.", "Torrent añadido.", "Torrent adicionado.", "Торрент добавлен.");
        case Str::CliAddFailure:
            return pick("Failed to add torrent.", "Aggiunta del torrent non riuscita.", "Échec de l'ajout du torrent.",
                        "Hinzufügen des Torrents fehlgeschlagen.", "No se pudo añadir el torrent.", "Falha ao adicionar torrent.", "Не удалось добавить торрент.");
        case Str::CliAddDuplicate:
            return pick("Torrent was already present.", "Il torrent era già presente.", "Le torrent était déjà présent.",
                        "Der Torrent war bereits vorhanden.", "El torrent ya estaba presente.", "O torrent já estava presente.", "Торрент уже был добавлен.");
        case Str::CliStartSuccess:
            return pick("Torrent started.", "Torrent avviato.", "Torrent démarré.", "Torrent gestartet.", "Torrent iniciado.", "Torrent iniciado.", "Торрент запущен.");
        case Str::CliStartFailure:
            return pick("Failed to start torrent.", "Avvio del torrent non riuscito.", "Échec du démarrage du torrent.",
                        "Starten des Torrents fehlgeschlagen.", "No se pudo iniciar el torrent.", "Falha ao iniciar torrent.", "Не удалось запустить торрент.");
        case Str::CliStopSuccess:
            return pick("Torrent stopped.", "Torrent fermato.", "Torrent arrêté.", "Torrent gestoppt.", "Torrent detenido.", "Torrent parado.", "Торрент остановлен.");
        case Str::CliStopFailure:
            return pick("Failed to stop torrent.", "Arresto del torrent non riuscito.", "Échec de l'arrêt du torrent.",
                        "Stoppen des Torrents fehlgeschlagen.", "No se pudo detener el torrent.", "Falha ao parar torrent.", "Не удалось остановить торрент.");
        case Str::CliRemoveSuccess:
            return pick("Torrent removed.", "Torrent rimosso.", "Torrent supprimé.", "Torrent entfernt.", "Torrent eliminado.", "Torrent removido.", "Торрент удалён.");
        case Str::CliRemoveFailure:
            return pick("Failed to remove torrent.", "Rimozione del torrent non riuscita.", "Échec de la suppression du torrent.",
                        "Entfernen des Torrents fehlgeschlagen.", "No se pudo eliminar el torrent.", "Falha ao remover torrent.", "Не удалось удалить торрент.");
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
