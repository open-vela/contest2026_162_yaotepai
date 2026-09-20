#!/bin/bash
# Flash nuttx.bin to SF32LB58 board via J-Link
# Usage: flash.sh [path/to/nuttx.bin]

set -e

WORKSPACE="/home/hongbo/Developer/Embedded/OpenVela"
JLINK_SCRIPT="$WORKSPACE/download_lb58_nand.jlink"

# Find binary
if [ -n "$1" ]; then
    NUTTX_BIN="$1"
else
    # Try CMake build first
    NUTTX_BIN=$(find "$WORKSPACE/cmake_out" -name "nuttx.bin" -newer "$WORKSPACE/nuttx/CMakeLists.txt" 2>/dev/null | head -1)
    if [ -z "$NUTTX_BIN" ]; then
        # Try Make build
        NUTTX_BIN="$WORKSPACE/nuttx/nuttx.bin"
    fi
fi

if [ ! -f "$NUTTX_BIN" ]; then
    echo "Error: nuttx.bin not found at $NUTTX_BIN"
    echo "Usage: $0 [path/to/nuttx.bin]"
    exit 1
fi

echo "Flashing: $NUTTX_BIN"
echo "JLink script: $JLINK_SCRIPT"

# Copy binary to workspace root (JLink script expects it there)
cp "$NUTTX_BIN" "$WORKSPACE/nuttx.bin"

# Flash
cd "$WORKSPACE"
JLinkExe -CommandFile "$JLINK_SCRIPT"
