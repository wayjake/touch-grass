/*
 * Game Entry Point for Native Build (macOS / Linux)
 *
 * Thin wrapper that includes the shared game logic and runs it in a
 * fixed-timestep loop against the terminal platform implementation.
 */

#include <stdlib.h>
#include <time.h>

// Platform abstraction
#include "../shared/platform.h"

// Arduino shim for game modules
#include "Arduino.h"

// Shared game logic
#include "../touch_grass/game.h"

// From platform_native.c
extern "C" bool platform_native_should_quit(void);

#define FRAME_MS 33  // ~30 FPS

int main(void) {
    srand((unsigned int)time(NULL));

    platform_init();
    game_setup();

    while (!platform_native_should_quit()) {
        unsigned long frameStart = platform_millis();

        game_loop();

        unsigned long elapsed = platform_millis() - frameStart;
        if (elapsed < FRAME_MS) {
            platform_delay((int)(FRAME_MS - elapsed));
        }
    }

    return 0;
}
