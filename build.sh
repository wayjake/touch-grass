#!/bin/bash
#
# Build TouchGrass (macOS / Linux)
#
# Requires a C compiler (clang or gcc). No other dependencies.
#
# Usage:
#   ./build.sh        # outputs ./touch-grass
#   ./touch-grass     # run in a terminal at least 128x33 characters
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CC="${CC:-cc}"

"$CC" \
    "$SCRIPT_DIR/src/main.c" \
    "$SCRIPT_DIR/src/platform_terminal.c" \
    -o "$SCRIPT_DIR/touch-grass" \
    -std=c11 -O2 -Wall

echo "Build complete: $SCRIPT_DIR/touch-grass"
echo ""
echo "Run it in a terminal at least 128x33 characters:"
echo "  $SCRIPT_DIR/touch-grass"
echo ""
echo "Controls: Arrows/WASD move, Z/Space/Enter = A, X/Esc = B, Q = quit"
