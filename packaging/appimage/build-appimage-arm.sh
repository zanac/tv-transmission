#!/usr/bin/env bash
# Build tv-transmission for ARM64 and assemble an AppDir.
# Final AppImage packaging is done on the x86_64 GitHub runner.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
MACHINE="$(uname -m)"

case "$MACHINE" in
    aarch64|arm64)
        ARCH="aarch64"
        ;;
    *)
        echo "Unsupported architecture: $MACHINE" >&2
        exit 1
        ;;
esac

BUILD_DIR="$PROJECT_ROOT/build-$ARCH"
APPDIR="$BUILD_DIR/AppDir"

echo "Building tv-transmission for $ARCH..."
rm -rf "$BUILD_DIR"
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Assembling AppDir..."
mkdir -p "$APPDIR/usr/bin"
cp "$BUILD_DIR/src/tv-transmission" "$APPDIR/usr/bin/"
cp "$SCRIPT_DIR/tv-transmission.desktop" "$APPDIR/tv-transmission.desktop"
cp "$SCRIPT_DIR/tv-transmission.png" "$APPDIR/tv-transmission.png"
ln -sf usr/bin/tv-transmission "$APPDIR/AppRun"

file "$APPDIR/usr/bin/tv-transmission" || true
