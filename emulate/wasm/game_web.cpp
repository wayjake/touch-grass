/*
 * Game Entry Point for Web Build
 *
 * Thin wrapper that includes the shared game logic and exports
 * game_setup/game_loop for the WASM module.
 */

#include <emscripten.h>
#include <stdlib.h>

// Platform abstraction
#include "../../shared/platform.h"

// Arduino shim for game modules
#include "Arduino.h"

// Shared game logic
#include "../../touch_grass/game.h"

// ============================================================================
// Exported Entry Points
// ============================================================================

extern "C" {

EMSCRIPTEN_KEEPALIVE
void game_init(void) {
    srand(42);  // Seed random (or use time-based for variety)
    platform_init();
    game_setup();
}

EMSCRIPTEN_KEEPALIVE
void game_tick(void) {
    game_loop();
}

} // extern "C"
