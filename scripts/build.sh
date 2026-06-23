#!/usr/bin/env bash
# Build script for HelloContact (Linux/macOS)
# Usage: ./scripts/build.sh [target]

set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

case "${1:-}" in
    clean)
        echo "[CLEAN] Removing $BUILD_DIR ..."
        rm -rf "$BUILD_DIR"
        echo "[CLEAN] Done."
        exit 0
        ;;
    terminal)
        EXTRA_FLAGS="-DHELLO_CONTACT_BUILD_DESKTOP=OFF"
        ;;
    desktop)
        EXTRA_FLAGS="-DHELLO_CONTACT_BUILD_DESKTOP=ON"
        ;;
    *)
        EXTRA_FLAGS="-DHELLO_CONTACT_BUILD_DESKTOP=ON"
        ;;
esac

echo "========================================"
echo "  HelloContact Build Script (Unix)"
echo "========================================"
echo ""

echo "[1/2] Configuring..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release $EXTRA_FLAGS .

echo ""
echo "[2/2] Building (jobs=$JOBS)..."
cmake --build "$BUILD_DIR" --config Release -j "$JOBS"

echo ""
echo "[DONE] Build successful!"
echo "  Terminal: $BUILD_DIR/apps/terminal/hello_contact_terminal"
echo "  Desktop:  $BUILD_DIR/apps/desktop/hello_contact_desktop"
