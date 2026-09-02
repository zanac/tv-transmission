#!/usr/bin/env bash
#
# Builds a self-contained AppImage for tv-transmission.
#
# Downloads linuxdeploy and appimagetool on first run (cached under
# packaging/appimage/tools/, gitignored) — both are needed to bundle the
# binary together with its shared library dependencies (libcurl,
# libncursesw, libgpm, and libcurl's own dependency tree) into a single
# file that runs on most x86_64 Linux distributions without installing
# anything.
#
# Usage:
#   ./packaging/appimage/build-appimage.sh
#
# Produces: build/TvTransmission-x86_64.AppImage

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TOOLS_DIR="$SCRIPT_DIR/tools"
APPDIR="$BUILD_DIR/AppDir"

mkdir -p "$TOOLS_DIR"

LINUXDEPLOY="$TOOLS_DIR/linuxdeploy-x86_64.AppImage"
APPIMAGETOOL="$TOOLS_DIR/appimagetool-x86_64.AppImage"

if [ ! -x "$LINUXDEPLOY" ]; then
    echo "Downloading linuxdeploy..."
    curl -fL -o "$LINUXDEPLOY" \
        https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
    chmod +x "$LINUXDEPLOY"
fi

if [ ! -x "$APPIMAGETOOL" ]; then
    echo "Downloading appimagetool..."
    curl -fL -o "$APPIMAGETOOL" \
        https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$APPIMAGETOOL"
fi

echo "Building tv-transmission (Release)..."
mkdir -p "$BUILD_DIR"
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Assembling AppDir..."
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp "$BUILD_DIR/src/tv-transmission" "$APPDIR/usr/bin/"

# linuxdeploy scans the given executable, copies every shared library it
# needs (except a standard exclude-list of libs assumed present on any
# target system — libc, libm, libgcc_s, libstdc++, ld-linux, etc.), sets
# each one's rpath to $ORIGIN so they resolve from inside the bundle
# instead of the host's, and wires up the desktop file/icon for desktop
# integration (Terminal=true in the .desktop file is what makes double-
# clicking the AppImage from a file manager open a terminal, since this
# is a TUI/CLI tool with no graphical window of its own).
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/tv-transmission" \
    --desktop-file "$SCRIPT_DIR/tv-transmission.desktop" \
    --icon-file "$SCRIPT_DIR/tv-transmission.png"

echo "Packaging AppImage..."
ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$BUILD_DIR/TvTransmission-x86_64.AppImage"

echo
echo "Done: $BUILD_DIR/TvTransmission-x86_64.AppImage"
