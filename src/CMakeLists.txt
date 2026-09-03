add_executable(tv-transmission
    main.cpp
    Config.cpp
    Obfuscation.cpp
    TextUtil.cpp
    cli/Cli.cpp
    rpc/TransmissionClient.cpp
    ui/App.cpp
    ui/TorrentListWindow.cpp
    ui/AddTorrentDialog.cpp
    ui/SettingsDialog.cpp
    ui/LanguageComboBox.cpp
    ui/WindowListDialog.cpp
    ui/AboutDialog.cpp
    ui/BandwidthStatusLine.cpp
    ui/TorrentDetailsWindow.cpp
    ui/TrackerListWindow.cpp
    ui/TrackerDetailWindow.cpp
    ui/Strings.cpp
)

target_include_directories(tv-transmission PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tv-transmission PRIVATE
    tvision
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

# Flags required by tvision (see magiblot/tvision's own README)
if(UNIX)
    target_compile_definitions(tv-transmission PRIVATE _XOPEN_SOURCE_EXTENDED)
endif()
