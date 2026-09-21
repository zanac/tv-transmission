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
// Eight-way now: `pt` (position 6, unchanged) is Brazilian Portuguese,
// `ptPt` (new, appended LAST rather than next to `pt`) is European
// Portuguese — appended at the end rather than inserted next to its
// sibling so Language::Portuguese could be given the next free enum
// value (7) without renumbering PortugueseBrazilian/Russian, which
// would silently reinterpret any already-saved settings.json's own
// numeric "language" field as the wrong language.
const char* pick(const char* en, const char* it, const char* fr, const char* de, const char* es,
                  const char* pt, const char* ru, const char* ptPt) {
    switch (g_language) {
        case Language::Italian: return it;
        case Language::French:  return fr;
        case Language::German:  return de;
        case Language::Spanish: return es;
        case Language::PortugueseBrazilian: return pt;
        case Language::Russian: return ru;
        case Language::Portuguese: return ptPt;
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
        case Str::MenuAdd:       return pick("~A~dd...", "~A~ggiungi...", "~A~jouter...", "~H~inzufügen...", "~A~ñadir...", "~A~dicionar...", "~Д~обавить...", "~A~dicionar...");
        case Str::MenuStart:     return pick("~S~tart", "~S~tart", "~D~émarrer", "~S~tarten", "~I~niciar", "~I~niciar", "~З~апустить", "~I~niciar");
        case Str::MenuStop:      return pick("S~t~op", "S~t~op", "Arrê~t~er", "S~t~oppen", "~D~etener", "~P~arar", "~О~становить", "~P~arar");
        case Str::MenuRemove:    return pick("~R~emove", "~R~imuovi", "~S~upprimer", "~E~ntfernen", "~E~liminar", "~R~emover", "~У~далить", "~R~emover");
        case Str::MenuConnection:
            return pick("~C~onnection...", "~C~onnessione...", "C~o~nnexion...",
                        "~V~erbindung...", "C~o~nexión...", "~C~onexão...", "Под~к~лючение...", "~L~igação...");
        case Str::MenuServerSettings:
            return pick("~S~erver...", "~S~erver...", "~S~erveur...", "~S~erver...", "~S~ervidor...", "~S~ervidor...", "~С~ервер...", "~S~ervidor...");
        case Str::MenuSessionStats:
            return pick("Session ~S~tatistics...", "~S~tatistiche sessione...", "~S~tatistiques de session...",
                        "Sitzungs~s~tatistik...", "E~s~tadísticas de sesión...", "Estatísticas da ~S~essão...", "~С~татистика сессии...", "Estatísticas da ~S~essão...");
        case Str::MenuQuit:      return pick("~Q~uit", "~E~sci", "~Q~uitter", "~B~eenden", "~S~alir", "~S~air", "~В~ыход", "~S~air");
        case Str::MenuSettingsMenu: return pick("~S~ettings", "~I~mpostazioni", "~P~aramètres", "~E~instellungen", "~C~onfiguración", "~C~onfigurações", "~Н~астройки", "~D~efinições");
        case Str::MenuColumnsMenu:
            return pick("~C~olumns", "~C~olonne", "~C~olonnes", "~S~palten", "Co~l~umnas", "~C~olunas", "Ст~о~лбцы", "~C~olunas");
        case Str::MenuConnectionsMenu:
            return pick("Con~n~ections", "Conn~e~ssioni", "Co~n~nexions", "~V~erbindungen", "Co~n~exiones", "Cone~x~ões", "Под~к~лючения", "Li~g~ações");
        case Str::MenuConnectionsEmpty:
            return pick("Empty", "Vuoto", "Vide", "Leer", "Vacío", "Vazio", "Пусто", "Vazio");
        case Str::MenuManageColumns:
            return pick("~M~anage columns...", "~G~estisci colonne...", "~G~érer les colonnes...",
                        "Spalten ~v~erwalten...", "~G~estionar columnas...", "~G~erenciar colunas...", "~У~правление столбцами...", "~G~erir colunas...");
        case Str::LabelFilterName:
            return pick("Name contains:", "Il nome contiene:", "Le nom contient :",
                        "Name enthält:", "El nombre contiene:", "O nome contém:", "Имя содержит:", "O nome contém:");
        case Str::LabelFilterStatusSection:
            return pick("Show status:", "Mostra stato:", "Afficher l'état :",
                        "Status anzeigen:", "Mostrar estado:", "Mostrar status:", "Показывать статус:", "Mostrar estado:");
        case Str::ButtonReset:
            return pick("Reset", "Ripristina", "Réinitialiser", "Zurücksetzen", "Restablecer", "Redefinir", "Сбросить", "Repor");
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
                "y la barra de estado cambien del todo (el resto ya ha cambiado).", "Idioma alterado. Reinicie o aplicativo para que a barra de menus e a barra de status mudem completamente (o resto já mudou).", "Язык изменён. Перезапустите приложение, чтобы строка меню и строка состояния переключились полностью (всё остальное уже изменено).", "Idioma alterado. Reinicie a aplicação para que a barra de menus e a barra de estado mudem por completo (o resto já mudou).");
        case Str::MsgServerAdded:
            return pick("Server '%s' added.", "Server '%s' aggiunto.", "Serveur « %s » ajouté.",
                        "Server '%s' hinzugefügt.", "Servidor '%s' añadido.", "Servidor '%s' adicionado.", "Сервер '%s' добавлен.", "Servidor '%s' adicionado.");
        case Str::MsgServerRemoved:
            return pick("Server '%s' removed.", "Server '%s' rimosso.", "Serveur « %s » supprimé.",
                        "Server '%s' entfernt.", "Servidor '%s' eliminado.", "Servidor '%s' removido.", "Сервер '%s' удалён.", "Servidor '%s' removido.");
        case Str::MsgConnectionTestFailed:
            return pick("Could not connect: %s", "Impossibile connettersi: %s", "Connexion impossible : %s",
                        "Verbindung fehlgeschlagen: %s", "No se pudo conectar: %s", "Não foi possível conectar: %s", "Не удалось подключиться: %s", "Não foi possível ligar: %s");
        case Str::DialogTitleColumnManager:
            return pick("Manage columns", "Gestisci colonne", "Gérer les colonnes",
                        "Spalten verwalten", "Gestionar columnas", "Gerenciar colunas", "Управление столбцами", "Gerir colunas");
        case Str::LabelColumnManagerColumn:
            return pick("Column", "Colonna", "Colonne", "Spalte", "Columna", "Coluna", "Столбец", "Coluna");
        case Str::LabelColumnManagerWidth:
            return pick("Width", "Larghezza", "Largeur", "Breite", "Ancho", "Largura", "Ширина", "Largura");
        case Str::LabelColumnManagerVisible:
            return pick("Visible", "Visibile", "Visible", "Sichtbar", "Visible", "Visível", "Видимый", "Visível");
        case Str::ButtonResizeColumn:
            return pick("~R~esize", "~R~idimensiona", "~R~edimensionner", "~G~röße ändern", "~R~edimensionar", "~R~edimensionar", "~И~зменить размер", "~R~edimensionar");
        case Str::ButtonMoveColumn:
            return pick("~M~ove", "~S~posta", "Dé~p~lacer", "~V~erschieben", "~M~over", "~M~over", "~П~ереместить", "~M~over");
        case Str::ButtonToggleVisible:
            return pick("~T~oggle visible", "~M~ostra/Nascondi", "~A~fficher/Masquer",
                        "S~i~chtbarkeit", "~O~cultar/Mostrar", "~M~ostrar/Ocultar", "~П~оказать/скрыть", "~M~ostrar/Ocultar");
        case Str::MenuVerify:      return pick("~V~erify", "~V~erifica", "~V~érifier", "~P~rüfen", "~V~erificar", "~V~erificar", "~П~роверить", "~V~erificar");
        case Str::MenuReannounce:  return pick("Reannoun~c~e", "Ricontatta tra~c~ker", "Réannon~c~er", "Tracker neu ~a~nfragen", "Reanun~c~iar", "Reanun~c~iar", "Пере~о~бъявить", "Reanun~c~iar");
        case Str::MenuStartNow:    return pick("Start ~N~ow", "Avvia s~u~bito", "Démarrer ~m~aintenant", "~J~etzt starten", "Iniciar ~a~hora", "Iniciar ~a~gora", "Запустить ~с~ейчас", "Iniciar ~a~gora");
        case Str::MenuShowDetails: return pick("~D~etails", "~D~ettagli", "Détai~l~s", "~D~etails", "Detal~l~es", "~D~etalhes", "~П~одробности", "~D~etalhes");
        case Str::MenuShowFiles:   return pick("~F~iles", "Fi~l~e", "~F~ichiers", "Date~i~en", "A~r~chivos", "~A~rquivos", "~Ф~айлы", "~F~icheiros");
        case Str::MenuSelectMultiple:
            return pick("Select ~M~ultiple", "Selezione ~m~ultipla", "Sélection ~m~ultiple",
                        "~M~ehrfachauswahl", "Selección ~m~últiple", "Seleção ~m~últipla", "~М~ножественный выбор", "Seleção ~m~últipla");
        case Str::MenuCancelSelection:
            return pick("~C~ancel selection", "~A~nnulla selezione", "~A~nnuler la sélection",
                        "Auswahl ~a~bbrechen", "~C~ancelar selección", "~C~ancelar seleção", "~О~тменить выбор", "~C~ancelar seleção");
        case Str::MenuQueue:
            return pick("Q~u~eue", "C~o~da", "F~i~le d'attente", "~W~arteschlange", "C~o~la", "F~i~la", "О~ч~ередь", "F~i~la");
        case Str::MenuQueueMoveTop:
            return pick("Move to ~T~op", "Porta in ~c~ima", "Déplacer tout en ~h~aut",
                        "~G~anz nach oben", "Mover al ~p~rincipio", "Mover para o ~t~opo", "Переместить нав~е~рх", "Mover para o ~t~opo");
        case Str::MenuQueueMoveUp:
            return pick("Move ~U~p", "Sposta ~s~u", "~M~onter",
                        "Nach ~o~ben", "Mover ~a~rriba", "Mover para ~c~ima", "Переместить в~в~ерх", "Mover para ~c~ima");
        case Str::MenuQueueMoveDown:
            return pick("Move ~D~own", "Sposta ~g~iù", "~D~escendre",
                        "Nach ~u~nten", "Mover a~b~ajo", "Mover para ~b~aixo", "Переместить в~н~из", "Mover para ~b~aixo");
        case Str::MenuQueueMoveBottom:
            return pick("Move to ~B~ottom", "Porta in ~f~ondo", "Déplacer tout en ~b~as",
                        "Ganz nach u~n~ten", "Mover al ~f~inal", "Mover para o ~f~im", "Переместить в конец", "Mover para o ~f~undo");
        case Str::MenuPriority:
            return pick("Priorit~y~", "Priorit~à~", "Priorit~é~", "P~r~iorität", "Prioridad", "Priorida~d~e", "П~р~иоритет", "Priorida~d~e");
        case Str::MenuPriorityLow:
            return pick("~L~ow", "~B~assa", "~B~asse", "~N~iedrig", "~B~aja", "~B~aixa", "~Н~изкий", "~B~aixa");
        case Str::MenuPriorityNormal:
            return pick("~N~ormal", "~N~ormale", "~N~ormale", "~N~ormal", "~N~ormal", "~N~ormal", "~О~бычный", "~N~ormal");
        case Str::MenuPriorityHigh:
            return pick("~H~igh", "~A~lta", "~H~aute", "~H~och", "~A~lta", "~A~lta", "~В~ысокий", "~A~lta");
        case Str::MenuDeleteWithData:
            return pick("Delete (~w~ith files)", "Elimina (con ~f~ile)", "~E~ffacer (avec fichiers)",
                        "~L~öschen (mit Dateien)", "~B~orrar (con archivos)", "Excluir (com ~a~rquivos)", "Удалить (с ~ф~айлами)", "Eliminar (com ~f~icheiros)");
        case Str::ConfirmRemoveTorrent:
            return pick("Remove '%s' from the list? Files on disk will be kept.",
                        "Rimuovere '%s' dalla lista? I file su disco resteranno.",
                        "Retirer '%s' de la liste ? Les fichiers sur le disque seront conservés.",
                        "'%s' aus der Liste entfernen? Die Dateien auf der Festplatte bleiben erhalten.",
                        "¿Quitar '%s' de la lista? Los archivos en el disco se conservarán.", "Remover '%s' da lista? Os arquivos no disco serão mantidos.", "Удалить '%s' из списка? Файлы на диске будут сохранены.", "Remover '%s' da lista? Os ficheiros no disco serão mantidos.");
        case Str::ConfirmDeleteTorrentWithData:
            return pick("Delete '%s' AND its files on disk? This cannot be undone.",
                        "Eliminare '%s' E i suoi file su disco? L'operazione non si puo' annullare.",
                        "Supprimer '%s' ET ses fichiers sur le disque ? Cette action est irréversible.",
                        "'%s' UND die zugehörigen Dateien löschen? Dies kann nicht rückgängig gemacht werden.",
                        "¿Eliminar '%s' Y sus archivos en el disco? Esta acción no se puede deshacer.", "Excluir '%s' E seus arquivos no disco? Esta ação não pode ser desfeita.", "Удалить '%s' И его файлы на диске? Это действие нельзя отменить.", "Eliminar '%s' E os seus ficheiros no disco? Esta ação não pode ser desfeita.");
        case Str::ConfirmRemoveTorrentsMulti:
            return pick("Remove %s torrents from the list? Files on disk will be kept.",
                        "Rimuovere %s torrent dalla lista? I file su disco resteranno.",
                        "Retirer %s torrents de la liste ? Les fichiers sur le disque seront conservés.",
                        "%s Torrents aus der Liste entfernen? Die Dateien auf der Festplatte bleiben erhalten.",
                        "¿Quitar %s torrents de la lista? Los archivos en el disco se conservarán.", "Remover %s torrents da lista? Os arquivos no disco serão mantidos.", "Удалить %s торрентов из списка? Файлы на диске будут сохранены.", "Remover %s torrents da lista? Os ficheiros no disco serão mantidos.");
        case Str::ConfirmDeleteTorrentsWithDataMulti:
            return pick("Delete %s torrents AND their files on disk? This cannot be undone.",
                        "Eliminare %s torrent E i loro file su disco? L'operazione non si puo' annullare.",
                        "Supprimer %s torrents ET leurs fichiers sur le disque ? Cette action est irréversible.",
                        "%s Torrents UND ihre Dateien löschen? Dies kann nicht rückgängig gemacht werden.",
                        "¿Eliminar %s torrents Y sus archivos en el disco? Esta acción no se puede deshacer.", "Excluir %s torrents E seus arquivos no disco? Esta ação não pode ser desfeita.", "Удалить %s торрентов И их файлы на диске? Это действие нельзя отменить.", "Eliminar %s torrents E os seus ficheiros no disco? Esta ação não pode ser desfeita.");

        // Standard tvision window-management menu: we just label items
        // that send tvision's own standard commands (cmZoom, cmNext,
        // cmClose, cmTile, cmCascade) — the logic already lives in
        // tvision itself (TWindow/TDeskTop/TApplication).
        case Str::MenuWindow:        return pick("~W~indow", "~F~inestra", "~F~enêtre", "~F~enster", "~V~entana", "~J~anela", "~О~кно", "~J~anela");
        case Str::MenuWindowZoom:    return "~Z~oom"; // same word in all five languages
        case Str::MenuWindowNext:    return pick("~N~ext", "~S~uccessiva", "~S~uivante", "~N~ächstes", "~S~iguiente", "Pró~x~ima", "~С~ледующее", "Segu~i~nte");
        case Str::MenuWindowClose:   return pick("~C~lose", "~C~hiudi", "~F~ermer", "~S~chließen", "~C~errar", "Fe~c~har", "~З~акрыть", "Fe~c~har");
        case Str::MenuWindowTile:    return pick("~T~ile", "~R~iquadra", "~M~osaïque", "~K~acheln", "~M~osaico", "~L~adrilho", "~М~озаика", "~M~osaico");
        case Str::MenuWindowCascade: return pick("C~a~scade", "Casc~a~ta", "Casc~a~de", "Kas~a~de", "Casc~a~da", "Casc~a~ta", "Кас~к~ад", "Casc~a~ta");
        case Str::MenuWindowList:    return pick("Window ~l~ist", "~E~lenco finestre", "~L~iste des fenêtres",
                                                  "~F~ensterliste", "~L~ista de ventanas", "~L~ista de janelas", "~С~писок окон", "~L~ista de janelas");
        case Str::MenuPanelsMenu:    return pick("~P~anels", "~P~annelli", "~P~anneaux",
                                                  "~P~anele", "~P~aneles", "~P~ainéis", "~П~анели", "~P~ainéis");
        case Str::MenuPanelStatus:   return pick("~S~tatus", "~S~tato", "É~t~at",
                                                  "~S~tatus", "~E~stado", "~S~tatus", "~С~татус", "~E~stado");
        case Str::MenuPanelFiles:    return pick("~F~iles", "~F~ile", "~F~ichiers",
                                                  "~D~ateien", "~A~rchivos", "~A~rquivos", "~Ф~айлы", "~F~icheiros");
        // Hotkeys deliberately NOT "S"/"F" here — those are already the
        // toggle items' own, in the same Panels submenu — picked from
        // within "Status"/"Files" itself instead, to stay unique within
        // that same menu without needing an unrelated letter.
        case Str::MenuPanelResizeStatus: return pick("Resize S~t~atus...", "Ridimensiona S~t~atus...", "Redimensionner É~t~at...",
                                                  "S~t~atus skalieren...", "Redimensionar Es~t~ado...", "Redimensionar S~t~atus...", "Размер: ~С~татус...", "Redimensionar Es~t~ado...");
        case Str::MenuPanelResizeFiles:  return pick("Resize F~i~les...", "Ridimensiona F~i~les...", "Redimensionner F~i~chiers...",
                                                  "Date~i~en skalieren...", "Redimensionar Arch~i~vos...", "Redimensionar Arqu~i~vos...", "Размер: ~Ф~айлы...", "Redimensionar F~i~cheiros...");
        // Separate from MenuWindowList on purpose: menu/status labels use
        // ~x~ markup to underline a hotkey letter, which only TMenuItem/
        // TStatusItem/TButton interpret. A TDialog title does NOT strip
        // it, so reusing the menu label here would show literal tildes
        // in the title bar.
        case Str::DialogTitleWindowList:
            return pick("Window list", "Elenco finestre", "Liste des fenêtres", "Fensterliste", "Lista de ventanas", "Lista de janelas", "Список окон", "Lista de janelas");

        case Str::MenuHelp:  return pick("~H~elp", "~A~iuto", "~A~ide", "~H~ilfe", "~A~yuda", "~A~juda", "~С~правка", "~A~juda");
        case Str::MenuAbout: return pick("~A~bout...", "~I~nfo su...", "À ~p~ropos...", "Ü~b~er...", "~A~cerca de...", "~S~obre...", "~О~ программе...", "~S~obre...");
        case Str::DialogTitleAbout:
            return pick("About", "Info su", "À propos", "Über", "Acerca de", "Sobre", "О программе", "Sobre");
        case Str::LabelAboutVersion:
            return pick("Version: %s", "Versione: %s", "Version : %s", "Version: %s", "Versión: %s", "Versão: %s", "Версия: %s", "Versão: %s");
        case Str::LabelAboutCopyright:
            return "Copyright © %d %s"; // same convention in all five languages

        case Str::StatusAdd:      return pick("~F2~ Add", "~F2~ Aggiungi", "~F2~ Ajouter", "~F2~ Hinzufügen", "~F2~ Añadir", "~F2~ Adicionar", "~F2~ Добавить", "~F2~ Adicionar");
        case Str::StatusStart:    return "~F5~ Start"; // same word in all five languages
        case Str::StatusStop:     return pick("~F6~ Stop", "~F6~ Stop", "~F6~ Arrêter", "~F6~ Stopp", "~F6~ Detener", "~F6~ Parar", "~F6~ Стоп", "~F6~ Parar");
        case Str::StatusSettings: return pick("~F9~ Connection", "~F9~ Connessione", "~F9~ Connexion", "~F9~ Verbindung", "~F9~ Conexión", "~F9~ Conexão", "~F9~ Соединение", "~F9~ Ligação");
        case Str::StatusQuit:     return pick("~Alt-X~ Quit", "~Alt-X~ Esci", "~Alt-X~ Quitter", "~Alt-X~ Beenden", "~Alt-X~ Salir", "~Alt-X~ Sair", "~Alt-X~ Выход", "~Alt-X~ Sair");

        case Str::WindowTitleTorrentList:
            return pick("Torrents", "Torrent", "Torrents", "Torrents", "Torrents", "Torrents", "Торренты", "Torrents");
        case Str::WindowTitleOffline:
            return pick("(offline)", "(offline)", "(hors ligne)", "(offline)", "(sin conexión)", "(offline)", "(офлайн)", "(offline)");

        case Str::DialogTitleAddTorrent:
            return pick("Add torrent", "Aggiungi torrent", "Ajouter un torrent", "Torrent hinzufügen", "Añadir torrent", "Adicionar torrent", "Добавить торрент", "Adicionar torrent");
        case Str::LabelAddTorrentUrl:
            return pick("Magnet link, .torrent URL or local path:",
                        "Magnet link, URL .torrent o path locale:",
                        "Lien magnet, URL .torrent ou chemin local :",
                        "Magnet-Link, .torrent-URL oder lokaler Pfad:",
                        "Enlace magnet, URL .torrent o ruta local:", "Link magnet, URL .torrent ou caminho local:", "Magnet-ссылка, URL .torrent или локальный путь:", "Ligação magnet, URL .torrent ou caminho local:");
        case Str::LabelFolderPath:
            return pick("Path:", "Percorso:", "Chemin :", "Pfad:", "Ruta:", "Caminho:", "Путь:", "Caminho:");
        case Str::MsgFolderUnreadable:
            return pick("(cannot read this directory)", "(impossibile leggere questa cartella)",
                        "(impossible de lire ce dossier)", "(Ordner kann nicht gelesen werden)",
                        "(no se puede leer esta carpeta)", "(não é possível ler esta pasta)", "(не удаётся прочитать эту папку)", "(não é possível ler esta pasta)");
        case Str::MsgTorrentDuplicate:
            return pick("This torrent was already in the list.",
                        "Questo torrent era già presente nella lista.",
                        "Ce torrent était déjà dans la liste.",
                        "Dieser Torrent war bereits in der Liste.",
                        "Este torrent ya estaba en la lista.", "Este torrent já estava na lista.", "Этот торрент уже был в списке.", "Este torrent já estava na lista.");
        case Str::MsgTorrentAddFailed:
            return pick("Failed to add the torrent:\n%s",
                        "Aggiunta del torrent non riuscita:\n%s",
                        "Échec de l'ajout du torrent :\n%s",
                        "Hinzufügen des Torrents fehlgeschlagen:\n%s",
                        "No se pudo añadir el torrent:\n%s", "Falha ao adicionar o torrent:\n%s", "Не удалось добавить торрент:\n%s", "Falha ao adicionar o torrent:\n%s");
        case Str::ButtonRename:
            return pick("Rename", "Rinomina", "Renommer", "Umbenennen", "Renombrar", "Renomear", "Переименовать", "Mudar nome");
        case Str::DialogTitleRename:
            return pick("Rename", "Rinomina", "Renommer", "Umbenennen", "Renombrar", "Renomear", "Переименовать", "Mudar nome");
        case Str::LabelRenameNewName:
            return pick("New name:", "Nuovo nome:", "Nouveau nom :", "Neuer Name:", "Nuevo nombre:", "Novo nome:", "Новое имя:", "Novo nome:");
        case Str::MsgRenameFailed:
            return pick("Failed to rename:\n%s",
                        "Rinomina non riuscita:\n%s",
                        "Échec du renommage :\n%s",
                        "Umbenennen fehlgeschlagen:\n%s",
                        "No se pudo renombrar:\n%s", "Falha ao renomear:\n%s", "Не удалось переименовать:\n%s", "Falha ao mudar o nome:\n%s");

        case Str::ButtonOK:     return "OK"; // same word in all five languages
        case Str::ButtonCancel: return pick("Cancel", "Annulla", "Annuler", "Abbrechen", "Cancelar", "Cancelar", "Отмена", "Cancelar");
        case Str::ButtonSave: return pick("Save", "Salva", "Enregistrer", "Speichern", "Guardar", "Salvar", "Сохранить", "Guardar");
        case Str::ButtonSelect: return pick("Select", "Seleziona", "Sélectionner", "Auswählen", "Seleccionar", "Selecionar", "Выбрать", "Selecionar");

        case Str::DialogTitleConnection:
            return pick("Connection", "Connessione", "Connexion", "Verbindung", "Conexión", "Conexão", "Соединение", "Ligação");
        case Str::DialogTitleServerSettings:
            return pick("Server Configuration", "Configurazione server", "Configuration du serveur",
                        "Server-Konfiguration", "Configuración del servidor", "Configuração do servidor", "Настройки сервера", "Configuração do servidor");
        case Str::DialogTitleSessionStats:
            return pick("Session Statistics", "Statistiche sessione", "Statistiques de session",
                        "Sitzungsstatistik", "Estadísticas de sesión", "Estatísticas da sessão", "Статистика сессии", "Estatísticas da sessão");
        case Str::DialogTitleSelectFolder:
            return pick("Select Folder", "Seleziona cartella", "Sélectionner un dossier",
                        "Ordner auswählen", "Seleccionar carpeta", "Selecionar pasta", "Выбрать папку", "Selecionar pasta");
        case Str::LabelRefreshSeconds:
            return pick("Refresh (seconds):", "Refresh (secondi):", "Actualisation (secondes) :",
                        "Aktualisierung (Sekunden):", "Actualización (segundos):", "Atualização (segundos):", "Обновление (секунды):", "Atualização (segundos):");
        case Str::LabelHost:
            return pick("Transmission host:", "Host Transmission:", "Hôte Transmission :",
                        "Transmission-Host:", "Host de Transmission:", "Host do Transmission:", "Хост Transmission:", "Anfitrião do Transmission:");
        case Str::LabelPort:
            return pick("RPC port:", "Porta RPC:", "Port RPC :", "RPC-Port:", "Puerto RPC:", "Porta RPC:", "Порт RPC:", "Porta RPC:");
        case Str::LabelRpcPath:
            return pick("RPC path:", "Percorso RPC:", "Chemin RPC :", "RPC-Pfad:", "Ruta RPC:", "Caminho RPC:", "Путь RPC:", "Caminho RPC:");
        case Str::LabelUser:
            return pick("User (optional):", "Utente (opzionale):", "Utilisateur (facultatif) :",
                        "Benutzer (optional):", "Usuario (opcional):", "Usuário (opcional):", "Пользователь (необязательно):", "Utilizador (opcional):");
        case Str::LabelPassword:
            return pick("Password (optional):", "Password (opzionale):", "Mot de passe (facultatif) :",
                        "Passwort (optional):", "Contraseña (opcional):", "Senha (opcional):", "Пароль (необязательно):", "Palavra-passe (opcional):");
        case Str::LabelLanguage:
            return pick("Language:", "Lingua:", "Langue :", "Sprache:", "Idioma:", "Idioma:", "Язык:", "Idioma:");
        case Str::LabelServerName:
            return pick("Server name:", "Nome server:", "Nom du serveur :",
                        "Servername:", "Nombre del servidor:", "Nome do servidor:", "Имя сервера:", "Nome do servidor:");
        case Str::DefaultServerName:
            return pick("server", "server", "serveur", "Server", "servidor", "servidor", "сервер", "servidor");
        // Native language names: identical regardless of the currently
        // selected language, so they stay recognizable to someone
        // looking for their own language in the list.
        case Str::LanguageEnglish: return "English";
        case Str::LanguageItalian: return "Italiano";
        case Str::LanguageFrench:  return "Français";
        case Str::LanguageGerman:  return "Deutsch";
        case Str::LanguageSpanish: return "Español";
        case Str::LanguagePortugueseBrazilian: return "Português (Brasil)";
        case Str::LanguagePortuguese: return "Português";
        case Str::LanguageRussian: return "Русский";

        case Str::WindowTitleDetails:
            return pick("Torrent details", "Dettagli torrent", "Détails du torrent", "Torrent-Details", "Detalles del torrent", "Detalhes do torrent", "Сведения о торренте", "Detalhes do torrent");
        case Str::LabelName:
            return pick("Name: %s", "Nome: %s", "Nom : %s", "Name: %s", "Nombre: %s", "Nome: %s", "Имя: %s", "Nome: %s");
        case Str::LabelSize:
            return pick("Size: %s", "Dimensione: %s", "Taille : %s", "Größe: %s", "Tamaño: %s", "Tamanho: %s", "Размер: %s", "Tamanho: %s");
        case Str::LabelCompleted:
            return pick("Completed: %.1f%%", "Completato: %.1f%%", "Terminé : %.1f%%",
                        "Abgeschlossen: %.1f%%", "Completado: %.1f%%", "Concluído: %.1f%%", "Завершено: %.1f%%", "Concluído: %.1f%%");
        case Str::LabelDownload:  return "Download: %.1f KB/s"; // "Download" is the same word in all five here
        case Str::LabelUpload:    return "Upload: %.1f KB/s";   // same reasoning
        case Str::LabelStatus:
            return pick("Status: %s", "Stato: %s", "État : %s", "Status: %s", "Estado: %s", "Status: %s", "Статус: %s", "Estado: %s");
        case Str::LabelError:
            return pick("Error: %s", "Errore: %s", "Erreur : %s", "Fehler: %s", "Error: %s", "Erro: %s", "Ошибка: %s", "Erro: %s");
        case Str::LabelId:        return "ID: %d"; // same in all five
        case Str::LabelAdded:
            return pick("Added: %s", "Aggiunto: %s", "Ajouté : %s", "Hinzugefügt: %s", "Añadido: %s", "Adicionado: %s", "Добавлено: %s", "Adicionado: %s");

        case Str::LabelLocation:
            return pick("Location: %s", "Posizione: %s", "Emplacement : %s", "Speicherort: %s", "Ubicación: %s", "Local: %s", "Расположение: %s", "Localização: %s");
        case Str::LabelPrivacyPublic:
            return pick("Privacy: public torrent", "Privacy: torrent pubblico", "Confidentialité : torrent public",
                        "Datenschutz: öffentlicher Torrent", "Privacidad: torrent público", "Privacidade: torrent público", "Конфиденциальность: публичный торрент", "Privacidade: torrent público");
        case Str::LabelPrivacyPrivate:
            return pick("Privacy: private torrent", "Privacy: torrent privato", "Confidentialité : torrent privé",
                        "Datenschutz: privater Torrent", "Privacidad: torrent privado", "Privacidade: torrent privado", "Конфиденциальность: приватный торрент", "Privacidade: torrent privado");
        case Str::LabelMagnet:    return "Magnet: %s"; // same word in all five
        case Str::LabelPieces:
            return pick("Pieces: %lld of %s", "Sezioni: %lld da %s", "Morceaux : %lld de %s",
                        "Teile: %lld von %s", "Piezas: %lld de %s", "Partes: %lld de %s", "Части: %lld из %s", "Partes: %lld de %s");

        case Str::LabelAvailable:
            return pick("Available: %.1f%%", "Disponibile: %.1f%%", "Disponible : %.1f%%",
                        "Verfügbar: %.1f%%", "Disponible: %.1f%%", "Disponível: %.1f%%", "Доступно: %.1f%%", "Disponível: %.1f%%");
        case Str::LabelDownloadedTotal:
            return pick("Downloaded: %s", "Scaricato: %s", "Téléchargé : %s", "Heruntergeladen: %s", "Descargado: %s", "Baixado: %s", "Скачано: %s", "Transferido: %s");
        case Str::LabelUploadedTotal:
            return pick("Uploaded: %s (Ratio: %.2f)", "Inviato: %s (Ratio: %.2f)", "Envoyé : %s (Ratio : %.2f)",
                        "Hochgeladen: %s (Verhältnis: %.2f)", "Subido: %s (Ratio: %.2f)", "Enviado: %s (Proporção: %.2f)", "Отдано: %s (Рейтинг: %.2f)", "Enviado: %s (Proporção: %.2f)");
        case Str::LabelAverageSpeed:
            return pick("Average speed: %s", "Velocità media: %s", "Vitesse moyenne : %s",
                        "Durchschnittsgeschwindigkeit: %s", "Velocidad media: %s", "Velocidade média: %s", "Средняя скорость: %s", "Velocidade média: %s");

        case Str::LabelLastActivity:
            return pick("Last activity: %s", "Ultima attività: %s", "Dernière activité : %s",
                        "Letzte Aktivität: %s", "Última actividad: %s", "Última atividade: %s", "Последняя активность: %s", "Última atividade: %s");

        case Str::LabelTimeDownloading:
            return pick("Downloading: %s", "In download: %s", "Téléchargement : %s", "Herunterladen: %s", "Descargando: %s", "Baixando: %s", "Скачивание: %s", "A transferir: %s");
        case Str::LabelTimeSeeding:
            return pick("Seeding: %s", "In seeding: %s", "Partage : %s", "Seeding: %s", "Compartiendo: %s", "Semeando: %s", "Раздача: %s", "A semear: %s");

        case Str::LabelSpeedLimitSection:
            return pick("Speed limit for this torrent:", "Limite di velocità per questo torrent:",
                        "Limite de vitesse pour ce torrent :", "Geschwindigkeitslimit für diesen Torrent:",
                        "Límite de velocidad para este torrent:", "Limite de velocidade para este torrent:", "Ограничение скорости для этого торрента:", "Limite de velocidade para este torrent:");
        case Str::LabelSeedRatioSection:
            return pick("Seed ratio limit for this torrent:", "Limite di rapporto per questo torrent:",
                        "Limite de partage pour ce torrent :", "Verhältnislimit für diesen Torrent:",
                        "Límite de ratio para este torrent:", "Limite de compartilhamento para este torrent:",
                        "Ограничение рейтинга для этого торрента:", "Limite de proporção para este torrent:");
        case Str::RadioSeedRatioGlobal:
            return pick("Use global setting", "Usa impostazione globale", "Utiliser le réglage global",
                        "Globale Einstellung verwenden", "Usar configuración global",
                        "Usar configuração global", "Использовать глобальную настройку", "Usar definição global");
        case Str::RadioSeedRatioCustom:
            return pick("Stop seeding at ratio:", "Ferma il seed al rapporto:", "Arrêter le partage au ratio :",
                        "Verteilen stoppen bei Verhältnis:", "Detener siembra en la proporción:",
                        "Parar de semear na proporção:", "Остановить раздачу при рейтинге:", "Parar de semear na proporção:");
        case Str::RadioSeedRatioUnlimited:
            return pick("Seed indefinitely", "Semina senza limiti", "Partager indéfiniment",
                        "Unbegrenzt verteilen", "Sembrar indefinidamente",
                        "Semear indefinidamente", "Раздавать без ограничений", "Semear indefinidamente");
        case Str::CheckLimitDownload:
            return pick("Limit download", "Limita download", "Limiter le téléchargement",
                        "Download begrenzen", "Limitar descarga", "Limitar download", "Ограничить скачивание", "Limitar transferência");
        case Str::CheckLimitUpload:
            return pick("Limit upload", "Limita upload", "Limiter l'envoi", "Upload begrenzen", "Limitar subida", "Limitar envio", "Ограничить отдачу", "Limitar envio");
        case Str::UnitKBs: return "KB/s"; // same in all five
        case Str::CheckHonorGlobalLimits:
            return pick("Honor global speed limits", "Rispetta i limiti globali",
                        "Respecter les limites globales", "Globale Limits berücksichtigen",
                        "Respetar los límites globales", "Respeitar limites globais", "Учитывать глобальные ограничения", "Respeitar limites globais");
        case Str::ButtonApply: return pick("Apply", "Applica", "Appliquer", "Anwenden", "Aplicar", "Aplicar", "Применить", "Aplicar");
        // Distinct from MenuWindowClose on purpose: same reason as
        // DialogTitleWindowList above (menu labels carry ~x~ hotkey
        // markup that a TButton here doesn't need and would show as
        // literal tildes if reused verbatim... actually TButton DOES
        // interpret ~x~, but keeping a separate plain string here avoids
        // any accidental coupling between this window's button and the
        // Window menu's Close item, which apply to different things
        // (this torrent's window vs. "whichever window is active").
        case Str::ButtonClose: return pick("Close", "Chiudi", "Fermer", "Schließen", "Cerrar", "Fechar", "Закрыть", "Fechar");
        case Str::ButtonTrackers: return pick("Trackers...", "Tracker...", "Trackers...", "Tracker...", "Trackers...", "Trackers...", "Трекеры...", "Trackers...");
        case Str::ButtonRefresh: return pick("Refresh", "Aggiorna", "Actualiser", "Aktualisieren", "Actualizar", "Atualizar", "Обновить", "Atualizar");
        case Str::ButtonBrowse: return pick("Browse...", "Sfoglia...", "Parcourir...", "Durchsuchen...", "Examinar...", "Procurar...", "Обзор...", "Procurar...");
        case Str::ButtonChangeFolder: return pick("Change...", "Cambia...", "Changer...", "Ändern...", "Cambiar...", "Alterar...", "Изменить...", "Alterar...");
        case Str::ButtonVerify: return pick("Verify", "Verifica", "Vérifier", "Prüfen", "Verificar", "Verificar", "Проверить", "Verificar");
        case Str::TabTrackers: return pick("Trackers", "Tracker", "Trackers", "Trackers", "Trackers", "Trackers", "Трекеры", "Trackers");
        case Str::TabPeers: return pick("Peers", "Peer", "Pairs", "Peers", "Pares", "Peers", "Пиры", "Peers");
        case Str::DialogTitleBrowseTorrent:
            return pick("Select a .torrent file", "Seleziona un file .torrent",
                        "Sélectionner un fichier .torrent", "Wähle eine .torrent-Datei",
                        "Selecciona un archivo .torrent", "Selecionar um arquivo .torrent", "Выберите файл .torrent", "Selecionar um ficheiro .torrent");

        case Str::WindowTitleTrackerList:
            return pick("Trackers: %s", "Tracker: %s", "Trackers : %s", "Tracker: %s", "Trackers: %s", "Trackers: %s", "Трекеры: %s", "Trackers: %s");
        case Str::WindowTitleTrackerDetail:
            return pick("Tracker details", "Dettagli tracker", "Détails du tracker", "Tracker-Details", "Detalles del tracker", "Detalhes do tracker", "Сведения о трекере", "Detalhes do tracker");
        case Str::HeaderTrackerHost: return "Tracker"; // same word in all five
        case Str::HeaderTier:        return "Tier"; // kept as-is in all five (Transmission's own term)
        case Str::HeaderSeeders:     return "Seeders"; // same word in all five
        case Str::HeaderLeechers:    return "Leechers"; // same word in all five
        case Str::HeaderDownloaded:
            return pick("Downloaded", "Scaricati", "Téléchargé", "Heruntergeladen", "Descargado", "Baixado", "Скачано", "Transferido");
        case Str::HeaderTrackerStatus:
            return pick("Status", "Stato", "État", "Status", "Estado", "Status", "Статус", "Estado");
        case Str::HeaderPeerAddress:
            return pick("Address", "Indirizzo", "Adresse", "Adresse", "Dirección", "Endereço", "Адрес", "Endereço");
        case Str::HeaderPeerClient:
            return pick("Client", "Client", "Client", "Client", "Cliente", "Cliente", "Клиент", "Cliente");
        case Str::HeaderPeerProgress:
            return pick("Progress", "Avanzamento", "Progression", "Fortschritt", "Progreso", "Progresso", "Прогресс", "Progresso");
        case Str::HeaderPeerDown:
            return pick("Down", "Ricezione", "Réception", "Empfang", "Recepción", "Recepção", "Приём", "Receção");
        case Str::HeaderPeerUp:
            return pick("Up", "Invio", "Envoi", "Sendung", "Envío", "Envio", "Отдача", "Envio");
        case Str::HeaderPeerFlags:
            return pick("Flags", "Flag", "Indicateurs", "Flags", "Indicadores", "Sinalizadores", "Флаги", "Sinalizadores");
        case Str::TrackerStatusOk: return "OK"; // same in all five
        case Str::TrackerStatusError:
            return pick("Error", "Errore", "Erreur", "Fehler", "Error", "Erro", "Ошибка", "Erro");
        case Str::ValueNotAvailable: return "N/A"; // same in all five
        case Str::LabelTrackerHost: return "Tracker: %s"; // same word in all five
        case Str::LabelTrackerTier: return "Tier: %d"; // same in all five
        case Str::LabelTrackerSeeders: return "Seeders: %s"; // same in all five
        case Str::LabelTrackerLeechers: return "Leechers: %s"; // same in all five
        case Str::LabelTrackerDownloaded:
            return pick("Downloaded: %s", "Scaricati: %s", "Téléchargé : %s", "Heruntergeladen: %s", "Descargado: %s", "Baixado: %s", "Скачано: %s", "Transferido: %s");
        case Str::LabelTrackerLastAnnounce:
            return pick("Last announce: %s", "Ultimo annuncio: %s", "Dernière annonce : %s",
                        "Letzte Ankündigung: %s", "Último anuncio: %s", "Último anúncio: %s", "Последнее объявление: %s", "Último anúncio: %s");
        case Str::LabelTrackerNextAnnounce:
            return pick("Next announce: %s", "Prossimo annuncio: %s", "Prochaine annonce : %s",
                        "Nächste Ankündigung: %s", "Próximo anuncio: %s", "Próximo anúncio: %s", "Следующее объявление: %s", "Próximo anúncio: %s");
        case Str::LabelTrackerResult:
            return pick("Result: %s", "Risultato: %s", "Résultat : %s", "Ergebnis: %s", "Resultado: %s", "Resultado: %s", "Результат: %s", "Resultado: %s");

        case Str::LabelGlobalSpeedSection:
            return pick("Global speed limit (all torrents):", "Limite di velocità globale (tutti i torrent):",
                        "Limite de vitesse globale (tous les torrents) :",
                        "Globales Geschwindigkeitslimit (alle Torrents):",
                        "Límite de velocidad global (todos los torrents):", "Limite de velocidade global (todos os torrents):", "Глобальное ограничение скорости (все торренты):", "Limite de velocidade global (todos os torrents):");
        case Str::CheckGlobalLimitDownload:
            return pick("Limit download", "Limita download", "Limiter le téléchargement",
                        "Download begrenzen", "Limitar descarga", "Limitar download", "Ограничить скачивание", "Limitar transferência");
        case Str::CheckGlobalLimitUpload:
            return pick("Limit upload", "Limita upload", "Limiter l'envoi", "Upload begrenzen", "Limitar subida", "Limitar envio", "Ограничить отдачу", "Limitar envio");
        case Str::LabelAltSpeedSection:
            return pick("'Speed Limit' mode", "Modalità 'Limite velocità'", "Mode « Limite de vitesse »",
                        "Modus 'Geschwindigkeitslimit'", "Modo 'Límite de velocidad'", "Modo 'Limite de velocidade'", "Режим «Ограничение скорости»", "Modo 'Limite de velocidade'");
        case Str::LabelAltSpeedDescription:
            return pick(
                "When enabled, 'Speed Limit' mode overrides\nthe global bandwidth limit",
                "Quando abilitata, la modalità 'Limite velocità'\nannulla la limitazione globale di banda",
                "Lorsqu'il est activé, le mode « Limite de vitesse »\nremplace la limite de bande passante globale",
                "Wenn aktiviert, setzt der Modus 'Geschwindigkeitslimit'\ndas globale Bandbreitenlimit außer Kraft",
                "Cuando está habilitado, el modo 'Límite de velocidad'\nanula el límite de ancho de banda global", "Quando ativado, o modo 'Limite de velocidade'\nsubstitui o limite global de banda", "Если включён, режим «Ограничение скорости»\nзаменяет глобальное ограничение полосы", "Quando ativado, o modo 'Limite de velocidade'\nsubstitui o limite global de largura de banda");
        case Str::LabelAltSpeedDownload:
            return pick("Download limit:", "Limite download:", "Limite de téléchargement :",
                        "Download-Limit:", "Límite de descarga:", "Limite de download:", "Ограничение скачивания:", "Limite de transferência:");
        case Str::LabelAltSpeedUpload:
            return pick("Upload limit:", "Limite upload:", "Limite d'envoi :",
                        "Upload-Limit:", "Límite de subida:", "Limite de envio:", "Ограничение отдачи:", "Limite de envio:");
        case Str::LabelNetworkSection:
            return pick("Network", "Rete", "Réseau", "Netzwerk", "Red", "Rede", "Сеть", "Rede");
        case Str::ButtonTestPort:
            return pick("Test port", "Test porta", "Tester le port", "Port testen", "Probar puerto", "Testar porta", "Проверить порт", "Testar porta");
        case Str::ButtonUpdateBlocklist:
            return pick("Update blocklist", "Aggiorna blocklist", "MAJ liste noire",
                        "Sperrliste aktualisieren", "Actualizar lista de bloqueo", "Atualizar lista de bloqueio", "Обновить чёрный список", "Atualizar lista de bloqueio");
        case Str::LabelCurrentSession:
            return pick("Current session", "Sessione corrente", "Session en cours",
                        "Aktuelle Sitzung", "Sesión actual", "Sessão atual", "Текущая сессия", "Sessão atual");
        case Str::LabelAllTime:
            return pick("All time", "Sempre", "Total", "Insgesamt", "Total", "Total", "За всё время", "Total");
        case Str::LabelStatsDownloaded:
            return pick("Downloaded:", "Scaricato:", "Téléchargé :", "Heruntergeladen:", "Descargado:", "Baixado:", "Скачано:", "Transferido:");
        case Str::LabelStatsUploaded:
            return pick("Uploaded:", "Caricato:", "Envoyé :", "Hochgeladen:", "Subido:", "Enviado:", "Отдано:", "Enviado:");
        case Str::LabelStatsActive:
            return pick("Active:", "Attivo:", "Actif :", "Aktiv:", "Activo:", "Ativo:", "Активно:", "Ativo:");
        case Str::LabelStatsStarted:
            return pick("Started %d times", "Avviato %d volte", "Démarré %d fois",
                        "%d Mal gestartet", "Iniciado %d veces", "Iniciado %d vezes", "Запущено %d раз", "Iniciado %d vezes");
        case Str::ResultPortOpen:
            return pick("Port: open", "Porta: aperta", "Port : ouvert", "Port: offen", "Puerto: abierto", "Porta: aberta", "Порт: открыт", "Porta: aberta");
        case Str::ResultPortClosed:
            return pick("Port: closed", "Porta: chiusa", "Port : fermé", "Port: geschlossen", "Puerto: cerrado", "Porta: fechada", "Порт: закрыт", "Porta: fechada");
        case Str::ResultPortTestFailed:
            return pick("Port: test failed", "Porta: test fallito", "Port : échec du test",
                        "Port: Test fehlgeschlagen", "Puerto: prueba fallida", "Porta: teste falhou", "Порт: ошибка проверки", "Porta: falha no teste");
        case Str::ResultBlocklistUpdated:
            return pick("Blocklist: %d rules", "Blocklist: %d regole", "Liste noire : %d règles",
                        "Sperrliste: %d Regeln", "Lista de bloqueo: %d reglas", "Lista de bloqueio: %d regras", "Чёрный список: %d правил", "Lista de bloqueio: %d regras");
        case Str::ResultBlocklistFailed:
            return pick("Blocklist: update failed", "Blocklist: aggiornamento fallito",
                        "Liste noire : échec", "Sperrliste: Fehler", "Lista de bloqueo: error", "Lista de bloqueio: falha na atualização", "Чёрный список: ошибка обновления", "Lista de bloqueio: falha na atualização");
        case Str::ResultTesting:
            return pick("Testing...", "Test in corso...", "Test en cours...", "Teste...", "Probando...", "Testando...", "Проверка...", "A testar...");
        case Str::ResultUpdating:
            return pick("Updating...", "Aggiornamento...", "Mise à jour...", "Aktualisiere...", "Actualizando...", "Atualizando...", "Обновление...", "A atualizar...");
        case Str::CheckAltSpeedEnabled:
            return pick("Enable 'Speed Limit' mode", "Abilita modalità 'Limite velocità'",
                        "Activer le mode « Limite de vitesse »", "Modus 'Geschwindigkeitslimit' aktivieren",
                        "Habilitar modo 'Límite de velocidad'", "Ativar modo 'Limite de velocidade'", "Включить режим «Ограничение скорости»", "Ativar modo 'Limite de velocidade'");

        case Str::TorrentStatusStopped:
            return pick("Stopped", "Fermo", "Arrêté", "Gestoppt", "Detenido", "Parado", "Остановлен", "Parado");
        case Str::TorrentStatusCheckWait:
            return pick("Queued for check", "In attesa di verifica", "En attente de vérification",
                        "Wartet auf Prüfung", "En espera de verificación", "Na fila para verificação", "В очереди на проверку", "Na fila para verificação");
        case Str::TorrentStatusChecking:
            return pick("Checking", "Verifica in corso", "Vérification en cours", "Wird geprüft", "Verificando", "Verificando", "Проверка", "A verificar");
        case Str::TorrentStatusDownloadWait:
            return pick("Queued for download", "In attesa di download", "En attente de téléchargement",
                        "Wartet auf Download", "En espera de descarga", "Na fila para download", "В очереди на скачивание", "Na fila para transferência");
        case Str::TorrentStatusDownloading:
            return pick("Downloading", "Download in corso", "Téléchargement en cours", "Wird heruntergeladen", "Descargando", "Baixando", "Скачивание", "A transferir");
        case Str::TorrentStatusSeedWait:
            return pick("Queued for seeding", "In attesa di seeding", "En attente de partage",
                        "Wartet auf Seeding", "En espera de compartir", "Na fila para semear", "В очереди на раздачу", "Na fila para semear");
        case Str::TorrentStatusSeeding:
            return pick("Seeding", "Seeding", "Partage", "Seeding", "Compartiendo", "Semeando", "Раздача", "A semear");
        case Str::TorrentStatusUnknown:
            return pick("Unknown", "Sconosciuto", "Inconnu", "Unbekannt", "Desconocido", "Desconhecido", "Неизвестно", "Desconhecido");

        // Torrent list column headers. "Download"/"Upload" stay the same
        // in all five languages: it's the same convention Transmission
        // itself uses in its own translated UIs.
        case Str::HeaderName:
            return pick("Name", "Nome", "Nom", "Name", "Nombre", "Nome", "Имя", "Nome");
        case Str::HeaderDone:
            return pick("Done", "Compl.", "Fait", "Fertig", "Hecho", "Progr.", "Готово", "Progr.");
        case Str::HeaderSize:
            return pick("Size", "Dim.", "Taille", "Größe", "Tamaño", "Tam.", "Разм.", "Tam.");
        case Str::HeaderDownload: return "Download"; // same word in all five
        case Str::HeaderUpload:   return "Upload"; // same word in all five
        case Str::HeaderId:       return "ID"; // same in all five
        case Str::HeaderStatus:
            return pick("Status", "Stato", "État", "Status", "Estado", "Status", "Статус", "Estado");
        case Str::HeaderAdded:
            return pick("Added", "Aggiunto", "Ajouté", "Hinzugefügt", "Añadido", "Adic.", "Добавл.", "Adic.");
        case Str::HeaderRatio:
            return pick("Ratio", "Rapporto", "Ratio", "Verhältnis", "Ratio", "Proporção", "Рейтинг", "Proporção");
        case Str::HeaderTotalUploaded:
            return pick("Uploaded", "Caricato", "Envoyé", "Hochgeladen", "Subido", "Enviado", "Отдано", "Enviado");
        case Str::HeaderTotalDownloaded:
            return pick("Downloaded", "Scaricato", "Reçu", "Heruntergeladen", "Descargado", "Baixado", "Скачано", "Transferido");
        case Str::HeaderLocation:
            return pick("Location", "Posizione", "Emplacement", "Speicherort", "Ubicación", "Local", "Расположение", "Localização");
        case Str::HeaderEta:       return "ETA"; // same abbreviation in all five
        case Str::HeaderPeers:
            return pick("Peers", "Peer", "Pairs", "Peers", "Pares", "Peers", "Пиры", "Peers");
        case Str::HeaderQueuePosition:
            return pick("Queue", "Coda", "File", "Warteschlange", "Cola", "Fila", "Очередь", "Fila");
        case Str::HeaderPriority:
            return pick("Priority", "Priorità", "Priorité", "Priorität", "Prioridad", "Prioridade", "Приоритет", "Prioridade");
        case Str::HeaderCompletedDate:
            return pick("Completed", "Completato", "Terminé", "Abgeschlossen", "Completado", "Concluído", "Завершено", "Concluído");
        case Str::PriorityLow:
            return pick("Low", "Bassa", "Basse", "Niedrig", "Baja", "Baixa", "Низкий", "Baixa");
        case Str::PriorityNormal:
            return pick("Normal", "Normale", "Normale", "Normal", "Normal", "Normal", "Обычный", "Normal");
        case Str::PriorityHigh:
            return pick("High", "Alta", "Haute", "Hoch", "Alta", "Alta", "Высокий", "Alta");

        case Str::WindowTitleFiles:
            return pick("Files: %s", "File: %s", "Fichiers : %s", "Dateien: %s", "Archivos: %s", "Arquivos: %s", "Файлы: %s", "Ficheiros: %s");
        case Str::HeaderFileName:
            return pick("File", "File", "Fichier", "Datei", "Archivo", "Arquivo", "Файл", "Ficheiro");
        case Str::HeaderFileSize:
            return pick("Size", "Dimensione", "Taille", "Größe", "Tamaño", "Tamanho", "Размер", "Tamanho");
        case Str::HeaderFileProgress:
            return pick("Done", "Fatto", "Terminé", "Fertig", "Hecho", "Feito", "Готово", "Feito");
        case Str::HeaderFileWanted:
            return pick("Wanted", "Richiesto", "Voulu", "Gewünscht", "Deseado", "Desejado", "Нужен", "Pretendido");
        case Str::HeaderFileEnable:
            // Same column FilesPanel's own wantedCol.header uses
            // (TorrentFilesWindow's own, separate window keeps
            // HeaderFileWanted above unchanged — asked for directly as
            // this panel's own column specifically, and that window
            // has more room for the longer word anyway).
            return pick("Enable", "Abilita", "Activer", "Aktivieren", "Habilitar", "Habilitar", "Включить", "Ativar");
        case Str::ValueMixed: return pick("Mixed", "Misto", "Mixte", "Gemischt", "Mixto", "Misto", "Смешанно", "Misto");
        case Str::ButtonToggleWanted:
            return pick("~T~oggle wanted", "~A~ttiva/disattiva", "Basc~u~ler voulu",
                        "~G~ewünscht umschalten", "Alternar de~s~eado", "~A~lternar desejado", "~П~ереключить", "~A~lternar pretendido");
        case Str::ButtonSelectAll:
            return pick("Select ~a~ll", "Seleziona ~t~utto", "~T~out sélectionner",
                        "~A~lle auswählen", "Seleccionar ~t~odo", "Selecionar ~t~udo", "Выбрать ~в~сё", "Selecionar ~t~udo");
        case Str::ButtonSelectNone:
            return pick("Select ~n~one", "Deseleziona t~u~tto", "Tout dé~s~électionner",
                        "Alle a~b~wählen", "Deseleccionar to~d~o", "Deseleci~o~nar tudo", "Снять ~в~ыделение", "Deseleci~o~nar tudo");
        case Str::ButtonPriorityLow:
            return pick("Priority ~L~ow", "Priorità ~b~assa", "Priorité ~b~asse",
                        "Priorität ~n~iedrig", "Prioridad ~b~aja", "Prioridade ~b~aixa", "Приоритет ~н~изкий", "Prioridade ~b~aixa");
        case Str::ButtonPriorityNormal:
            return pick("Priority ~N~ormal", "Priorità n~o~rmale", "Priorité n~o~rmale",
                        "Priorität n~o~rmal", "Prioridad n~o~rmal", "Prioridade n~o~rmal", "Приоритет об~ы~чный", "Prioridade n~o~rmal");
        case Str::ButtonPriorityHigh:
            return pick("Priority ~H~igh", "Priorità ~a~lta", "Priorité ~h~aute",
                        "Priorität ~h~och", "Prioridad ~a~lta", "Prioridade ~a~lta", "Приоритет выс~о~кий", "Prioridade ~a~lta");

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
                case Language::Portuguese: return
"Utilização: tv-transmission [opções globais] <comando> [argumentos]\n"
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
"  --host <anfitrião>   Anfitrião RPC do Transmission (padrão: das configurações guardadas)\n"
"  --port <porta>       Porta RPC do Transmission (padrão: das configurações guardadas)\n"
"  --rpc-path <caminho> Caminho RPC do Transmission (padrão: transmission/rpc)\n"
"  --user <utilizador>  Utilizador RPC (padrão: das configurações guardadas)\n"
"  --password <senha>   Palavra-passe RPC (padrão: das configurações guardadas)\n"
"  -h, --help           Mostra esta mensagem\n"
"\n"
"Por padrão, anfitrião/porta/utilizador/palavra-passe são lidos do\n"
"ficheiro de configurações guardadas (~/.config/tv-transmission/settings.json),\n"
"o mesmo que é definido a partir da janela de Definições da TUI, pelo que\n"
"não é preciso indicá-los em cada comando. Qualquer uma das opções\n"
"--host/--port/--rpc-path/--user/--password substitui apenas esse valor.";
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
                        "Fehlendes Argument: %s", "Falta el argumento: %s", "Argumento ausente: %s", "Отсутствует аргумент: %s", "Argumento em falta: %s");
        case Str::CliErrorUnknownCommand:
            return pick("Unknown command: %s", "Comando sconosciuto: %s", "Commande inconnue : %s",
                        "Unbekannter Befehl: %s", "Comando desconocido: %s", "Comando desconhecido: %s", "Неизвестная команда: %s", "Comando desconhecido: %s");
        case Str::CliErrorInvalidId:
            return pick("Invalid torrent id: %s", "ID torrent non valido: %s", "Identifiant de torrent invalide : %s",
                        "Ungültige Torrent-ID: %s", "ID de torrent no válido: %s", "ID de torrent inválido: %s", "Неверный ID торрента: %s", "ID de torrent inválido: %s");
        case Str::CliListEmpty:
            return pick("No torrents.", "Nessun torrent.", "Aucun torrent.", "Keine Torrents.", "Sin torrents.", "Nenhum torrent.", "Нет торрентов.", "Nenhum torrent.");
        case Str::CliAddSuccess:
            return pick("Torrent added.", "Torrent aggiunto.", "Torrent ajouté.", "Torrent hinzugefügt.", "Torrent añadido.", "Torrent adicionado.", "Торрент добавлен.", "Torrent adicionado.");
        case Str::CliAddFailure:
            return pick("Failed to add torrent.", "Aggiunta del torrent non riuscita.", "Échec de l'ajout du torrent.",
                        "Hinzufügen des Torrents fehlgeschlagen.", "No se pudo añadir el torrent.", "Falha ao adicionar torrent.", "Не удалось добавить торрент.", "Falha ao adicionar o torrent.");
        case Str::CliAddDuplicate:
            return pick("Torrent was already present.", "Il torrent era già presente.", "Le torrent était déjà présent.",
                        "Der Torrent war bereits vorhanden.", "El torrent ya estaba presente.", "O torrent já estava presente.", "Торрент уже был добавлен.", "O torrent já estava presente.");
        case Str::CliStartSuccess:
            return pick("Torrent started.", "Torrent avviato.", "Torrent démarré.", "Torrent gestartet.", "Torrent iniciado.", "Torrent iniciado.", "Торрент запущен.", "Torrent iniciado.");
        case Str::CliStartFailure:
            return pick("Failed to start torrent.", "Avvio del torrent non riuscito.", "Échec du démarrage du torrent.",
                        "Starten des Torrents fehlgeschlagen.", "No se pudo iniciar el torrent.", "Falha ao iniciar torrent.", "Не удалось запустить торрент.", "Falha ao iniciar o torrent.");
        case Str::CliStopSuccess:
            return pick("Torrent stopped.", "Torrent fermato.", "Torrent arrêté.", "Torrent gestoppt.", "Torrent detenido.", "Torrent parado.", "Торрент остановлен.", "Torrent parado.");
        case Str::CliStopFailure:
            return pick("Failed to stop torrent.", "Arresto del torrent non riuscito.", "Échec de l'arrêt du torrent.",
                        "Stoppen des Torrents fehlgeschlagen.", "No se pudo detener el torrent.", "Falha ao parar torrent.", "Не удалось остановить торрент.", "Falha ao parar o torrent.");
        case Str::CliRemoveSuccess:
            return pick("Torrent removed.", "Torrent rimosso.", "Torrent supprimé.", "Torrent entfernt.", "Torrent eliminado.", "Torrent removido.", "Торрент удалён.", "Torrent removido.");
        case Str::CliRemoveFailure:
            return pick("Failed to remove torrent.", "Rimozione del torrent non riuscita.", "Échec de la suppression du torrent.",
                        "Entfernen des Torrents fehlgeschlagen.", "No se pudo eliminar el torrent.", "Falha ao remover torrent.", "Не удалось удалить торрент.", "Falha ao remover o torrent.");
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
