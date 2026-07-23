/*
 * TouchGrass - terminal tile-based exploration game
 *
 * Entry point: runs the game state machine (game/game.h) in a
 * fixed-timestep loop against the terminal platform layer.
 */

#include <stdlib.h>
#include <time.h>

#include "platform.h"
#include "game/game.h"

#define FRAME_MS 33  // ~30 FPS

int main(void) {
    srand((unsigned int)time(NULL));

    platform_init();
    game_setup();

    while (!platform_should_quit()) {
        unsigned long frameStart = platform_millis();

        game_loop();

        unsigned long elapsed = platform_millis() - frameStart;
        if (elapsed < FRAME_MS) {
            platform_delay((int)(FRAME_MS - elapsed));
        }
    }

    return 0;
}
