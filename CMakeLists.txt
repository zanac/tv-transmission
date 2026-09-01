cmake_minimum_required(VERSION 3.16)
project(tv-transmission LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Turbo Vision (magiblot/tvision) as a submodule in external/tvision ---
# If missing, add it with:
#   git submodule add https://github.com/magiblot/tvision external/tvision
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/external/tvision/CMakeLists.txt")
    message(FATAL_ERROR
        "tvision not found in external/tvision.\n"
        "Run: git submodule add https://github.com/magiblot/tvision external/tvision\n"
        "     git submodule update --init --recursive")
endif()
add_subdirectory(external/tvision)

# --- libcurl, to talk to Transmission's RPC ---
find_package(CURL REQUIRED)

# --- nlohmann/json (header-only) ---
# Uses the system package if present (e.g. `apt install nlohmann-json3-dev`),
# otherwise downloads it via FetchContent.
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(json)
endif()

add_subdirectory(src)
