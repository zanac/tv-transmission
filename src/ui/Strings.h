#pragma once
#include <string>
#include "../AppSettings.h"

// Sets/reads the UI's current language. This is deliberately global
// state: the app is single-threaded, and most text lives in
// dialogs/windows that get rebuilt every time they're shown (Add
// torrent, Settings, Torrent details), so they always read the language
// "right now" without having to pass it around explicitly.
// Exception: the menu bar and status bar are built only once at startup
// (see App::initMenuBar/initStatusLine), so a runtime language change
// only relabels them after the app is restarted.
void setLanguage(Language lang);
Language currentLanguage();

// Identifier for every translatable piece of UI text.
enum class Str {
    MenuTorrent, MenuAdd, MenuStart, MenuStop, MenuRemove, MenuSettings, MenuQuit,
    MenuWindow, MenuWindowZoom, MenuWindowNext, MenuWindowClose,
    MenuWindowTile, MenuWindowCascade, MenuWindowList,
    DialogTitleWindowList,
    StatusAdd, StatusStart, StatusStop, StatusSettings, StatusQuit,

    WindowTitleTorrentList,

    DialogTitleAddTorrent, LabelAddTorrentUrl,
    ButtonOK, ButtonCancel,

    DialogTitleSettings,
    LabelRefreshSeconds, LabelHost, LabelPort, LabelUser, LabelPassword, LabelLanguage,
    LanguageEnglish, LanguageItalian,

    WindowTitleDetails,
    LabelName, LabelSize, LabelCompleted, LabelDownload, LabelUpload,
    LabelStatus, LabelError, LabelId,

    TorrentStatusStopped, TorrentStatusCheckWait, TorrentStatusChecking,
    TorrentStatusDownloadWait, TorrentStatusDownloading, TorrentStatusSeedWait,
    TorrentStatusSeeding, TorrentStatusUnknown,

    HeaderName, HeaderDone, HeaderSize, HeaderDownload, HeaderUpload,
    HeaderId, HeaderStatus,

    // Command-line interface (src/cli/Cli.cpp)
    CliUsage,
    CliErrorMissingArgument, CliErrorUnknownCommand, CliErrorInvalidId,
    CliListEmpty,
    CliAddSuccess, CliAddFailure,
    CliStartSuccess, CliStartFailure,
    CliStopSuccess, CliStopFailure,
    CliRemoveSuccess, CliRemoveFailure,
};

// Returns the translated text for the given id, in the current language.
const char* tr(Str id);

// Maps Transmission RPC's "status" field (tr_torrent_activity, 0-6) to
// the corresponding translated text.
const char* trTorrentStatus(int status);
