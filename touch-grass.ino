/*
 * ===============================================================================
 *
 *    _____                _       ____
 *   |_   _|__  _   _  ___| |__   / ___|_ __ __ _ ___ ___
 *     | |/ _ \| | | |/ __| '_ \ | |  _| '__/ _` / __/ __|
 *     | | (_) | |_| | (__| | | || |_| | | | (_| \__ \__ \
 *     |_|\___/ \__,_|\___|_| |_(_)____|_|  \__,_|___/___/
 *
 *   A tile-based exploration game for ESP32 + OLED
 *
 * ===============================================================================
 */

// Platform abstraction (must be first)
#include "shared/platform.h"
#include "shared/platform_esp32.h"

// Game logic (shared between ESP32 and Web)
#include "touch_grass/game.h"

void setup() {
    platform_init();
    game_setup();
}

void loop() {
    game_loop();
}
