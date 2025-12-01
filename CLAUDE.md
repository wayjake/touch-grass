# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Git Commits

- Do NOT include Claude watermarks or co-author tags in commit messages
- Keep commit messages concise and descriptive

## Build Commands

```bash
# Build firmware (outputs to build/)
arduino-cli compile --fqbn esp32:esp32:esp32s3 --output-dir build TouchGrass.ino

# Watch for changes and auto-rebuild
./watch.sh

# Run in Wokwi simulator
wokwi-cli .
```

## Architecture

TouchGrass is a tile-based exploration game for ESP32-S3 with SH1106 OLED display.

### File Structure

- `TouchGrass.ino` - Main game loop, state machine (STATE_WORLD, STATE_TILE_VIEW), input handling
- `shared/config.h` - Hardware pin definitions and display constants
- `shared/hardware.h` - Hardware abstraction: buttons (6-button d-pad + A/B), dual buzzers, RGB LED, display init
- `shared/graphics.h` - Sprite rendering utilities (drawTile, drawXbm)
- `shared/sound.h` - Audio: blocking melodies, non-blocking background music, beeps
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

### Button API

```cpp
button_pressed(BUTTON_A)   // Rising edge
button_held(BUTTON_UP)     // Currently down
dpad_up_pressed()          // Convenience for d-pad
```
