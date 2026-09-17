#!/usr/bin/env bash
# Build tv-transmission AppImage natively on ARM Linux.
# Supported architectures: aarch64 and armhf (armv7l).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
MACHINE="$(uname -m)"

case "$MACHINE" in
    aarch64|arm64)
        ARCH="aarch64"
        ;;
    armv7l|armv7*|armhf)
        ARCH="armhf"
        ;;
    *)
        echo "Unsupported ARM architecture: $MACHINE" >&2
        exit 1
        ;;
esac

BUILD_DIR="$PROJECT_ROOT/build-$ARCH"
TOOLS_DIR="$SCRIPT_DIR/tools-$ARCH"
APPDIR="$BUILD_DIR/AppDir"
OUTPUT="$BUILD_DIR/TvTransmission-linux-$ARCH.AppImage"

mkdir -p "$TOOLS_DIR"

LINUXDEPLOY_IMAGE="$TOOLS_DIR/linuxdeploy-$ARCH.AppImage"
APPIMAGETOOL_IMAGE="$TOOLS_DIR/appimagetool-$ARCH.AppImage"
LINUXDEPLOY_DIR="$TOOLS_DIR/linuxdeploy-extracted"
APPIMAGETOOL_DIR="$TOOLS_DIR/appimagetool-extracted"

if [ ! -f "$LINUXDEPLOY_IMAGE" ]; then
    curl -fL -o "$LINUXDEPLOY_IMAGE" \
        "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage"
    chmod +x "$LINUXDEPLOY_IMAGE"
fi

if [ ! -f "$APPIMAGETOOL_IMAGE" ]; then
    curl -fL -o "$APPIMAGETOOL_IMAGE" \
        "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$ARCH.AppImage"
    chmod +x "$APPIMAGETOOL_IMAGE"
fi

extract_appimage() {
    local image="$1"
    local dest="$2"

    rm -rf "$dest"
    mkdir -p "$dest"
    (
        cd "$dest"
        "$image" --appimage-extract >/dev/null
        mv squashfs-root/* .
        rmdir squashfs-root
    )
}

echo "Preparing ARM AppImage tools..."
extract_appimage "$LINUXDEPLOY_IMAGE" "$LINUXDEPLOY_DIR"
extract_appimage "$APPIMAGETOOL_IMAGE" "$APPIMAGETOOL_DIR"

LINUXDEPLOY="$LINUXDEPLOY_DIR/AppRun"
APPIMAGETOOL="$APPIMAGETOOL_DIR/AppRun"

if [ ! -x "$LINUXDEPLOY" ] || [ ! -x "$APPIMAGETOOL" ]; then
    echo "Failed to extract AppImage tools" >&2
    exit 1
fi

echo "Building tv-transmission for $ARCH..."
rm -rf "$BUILD_DIR"
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Assembling AppDir..."
mkdir -p "$APPDIR/usr/bin"
cp "$BUILD_DIR/src/tv-transmission" "$APPDIR/usr/bin/"

"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --executable "$APPDIR/usr/bin/tv-transmission" \
    --desktop-file "$SCRIPT_DIR/tv-transmission.desktop" \
    --icon-file "$SCRIPT_DIR/tv-transmission.png"

ARCH="$ARCH" "$APPIMAGETOOL" "$APPDIR" "$OUTPUT"
chmod +x "$OUTPUT"

echo "Done: $OUTPUT"
file "$APPDIR/usr/bin/tv-transmission" || true
ls -lh "$OUTPUT"
