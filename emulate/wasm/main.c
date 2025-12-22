/*
 * WASM Main Entry Point
 *
 * Minimal main() - actual game logic is in game_web.cpp
 */

int main(void) {
    // Don't initialize here - let JS call game_init() after audio context is ready
    return 0;
}
