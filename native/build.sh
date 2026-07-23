#!/bin/bash
#
# Build TouchGrass as a native terminal executable (macOS / Linux)
#
# Requires a C/C++ compiler (clang or gcc). No other dependencies.
#
# Usage:
#   ./build.sh          # outputs native/touch-grass
#   ./touch-grass       # run in a terminal at least 128x33 characters
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/.."
OUTPUT="$SCRIPT_DIR/touch-grass"

CC="${CC:-cc}"
CXX="${CXX:-c++}"

echo "Building TouchGrass native..."

# Platform layer (pure C)
"$CC" -c \
    "$SCRIPT_DIR/platform_native.c" \
    -I"$PROJECT_ROOT/shared" \
    -o "$SCRIPT_DIR/platform_native.o" \
    -O2 -Wall

# Game wrapper (C++, includes the shared game headers)
"$CXX" -c \
    "$SCRIPT_DIR/game_native.cpp" \
    -I"$SCRIPT_DIR" \
    -I"$PROJECT_ROOT/shared" \
    -I"$PROJECT_ROOT/touch_grass" \
    -DPLATFORM_NATIVE \
    -o "$SCRIPT_DIR/game_native.o" \
    -O2 -Wall

# Link
"$CXX" "$SCRIPT_DIR/platform_native.o" "$SCRIPT_DIR/game_native.o" -o "$OUTPUT"

rm -f "$SCRIPT_DIR/platform_native.o" "$SCRIPT_DIR/game_native.o"

echo ""
echo "Build complete: $OUTPUT"
echo ""
echo "Run it in a terminal at least 128x33 characters:"
echo "  $OUTPUT"
echo ""
echo "Controls: Arrows/WASD move, Z/Space/Enter = A, X/Esc = B, Q = quit"
