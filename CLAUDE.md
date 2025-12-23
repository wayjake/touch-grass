# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Git Commits

- Do NOT include Claude watermarks or co-author tags in commit messages
- Keep commit messages concise and descriptive

## Build Commands

```bash
# Build firmware (outputs to build/)
./build.sh

# Watch for changes and auto-rebuild (requires inotifywait)
./watch.sh

# Flash to ESP32-S3 device
./flash.sh

# Run in Wokwi simulator
wokwi-cli .
```

## Web Emulation

The game can run in a browser using WebAssembly:

```bash
# Build WASM (requires Emscripten SDK)
cd emulate/wasm && ./build.sh

# Install dependencies and run dev server
cd emulate && npm install && npm run dev
```

The emulator is available at `http://localhost:5173/play`.

**Controls:** Arrow keys (move), Z/Space (A button), X/Esc (B button)

### Emulator Architecture

- `emulate/` - React Router app with Tailwind CSS
- `emulate/wasm/` - WASM build files
  - `platform_web.c` - Web platform implementation
  - `game_web.cpp` - Game wrapper for WASM
  - `build.sh` - Emscripten build script (outputs to `emulate/public/wasm/`)
- `emulate/app/routes/play.tsx` - Game canvas and input handling

## Architecture

TouchGrass is a tile-based exploration game for ESP32-S3 with SH1106 OLED display.

### File Structure

- `touch-grass.ino` - Main game loop, state machine (STATE_WORLD, STATE_TILE_VIEW), input handling
- `shared/platform.h` - Platform abstraction API (display, input, sound, timing)
- `shared/platform_esp32.h` - ESP32 implementation of platform API
- `shared/config.h` - Hardware pin definitions and display constants (legacy)
- `shared/hardware.h` - Hardware abstraction (legacy, wrapped by platform_esp32.h)
- `shared/graphics.h` - Sprite rendering utilities (legacy, wrapped by platform_esp32.h)
- `shared/sound.h` - Audio utilities (legacy, wrapped by platform_esp32.h)
- `touch_grass/terrain.h` - Map generation (procedural rivers, dirt patches), player movement, tile state
- `touch_grass/sprites.h` - 8x8 tile sprites (TILE_GRASS, TILE_WATER, TILE_DIRT, TILE_CHAR) and tile type constants

### Hardware Configuration

| Component | GPIO |
|-----------|------|
| D-pad (UP/DOWN/LEFT/RIGHT) | 38/35/36/37 |
| Buttons A/B | 19/20 |
| Buzzers | 5, 40 |
| RGB LED (WS2812B) | 1 |
| OLED I2C | SDA:41, SCL:42 |

### Game State

The game uses a simple state machine:
- `STATE_WORLD`: Player moves on 16x8 tile grid, terrain rendered as letters, player as sprite
- `STATE_TILE_VIEW`: Zoomed view showing tile name, scaled sprite, action menu

Player position tracked separately from map via `tg_playerX`, `tg_playerY`, and `tg_underPlayer` (stores tile type under player).

### Platform API

```cpp
// Input
platform_button_pressed(BTN_A)   // Rising edge
platform_button_held(BTN_UP)     // Currently down
platform_dpad_up_pressed()       // Convenience for d-pad

// Display
platform_clear_screen()
platform_set_cursor(x, y)
platform_print("text")
platform_draw_tile(tileX, tileY, spriteData)
platform_render()

// Sound
platform_beep(freq, duration_ms)
platform_play_melody(notes, durations, len)
```
