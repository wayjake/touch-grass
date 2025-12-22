#!/bin/bash
#
# Build TouchGrass for Web using Emscripten
#
# Prerequisites:
#   - Install Emscripten: https://emscripten.org/docs/getting_started/downloads.html
#   - Activate emsdk: source /path/to/emsdk/emsdk_env.sh
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR/../.."
OUTPUT_DIR="$SCRIPT_DIR/../public/wasm"

echo "Building TouchGrass WASM..."
echo "  Project root: $PROJECT_ROOT"
echo "  Output dir: $OUTPUT_DIR"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Check for emcc
if ! command -v emcc &> /dev/null; then
    echo "Error: emcc not found. Please install and activate Emscripten SDK."
    echo "  Install: https://emscripten.org/docs/getting_started/downloads.html"
    echo "  Activate: source /path/to/emsdk/emsdk_env.sh"
    exit 1
fi

# Compile
emcc \
    "$SCRIPT_DIR/main.c" \
    "$SCRIPT_DIR/platform_web.c" \
    "$SCRIPT_DIR/game_web.cpp" \
    -I"$SCRIPT_DIR" \
    -I"$PROJECT_ROOT/shared" \
    -I"$PROJECT_ROOT/touch_grass" \
    -o "$OUTPUT_DIR/game.js" \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_main", "_game_init", "_game_tick", "_platform_get_framebuffer", "_platform_get_framebuffer_size", "_platform_set_button"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPU8"]' \
    -s EXPORT_ES6=0 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createGameModule" \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ENVIRONMENT='web' \
    -s NO_EXIT_RUNTIME=1 \
    -O2

echo ""
echo "Build complete!"
echo "  Output: $OUTPUT_DIR/game.js"
echo "  Output: $OUTPUT_DIR/game.wasm"
echo ""
echo "To use in React, import and initialize the module:"
echo "  import createGameModule from './wasm/game.js'"
echo "  const Module = await createGameModule()"
