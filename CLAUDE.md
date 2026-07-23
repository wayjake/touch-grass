# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Git Commits

- Do NOT include Claude watermarks or co-author tags in commit messages
- Keep commit messages concise and descriptive

## Build Commands

```bash
# Build (outputs ./touch-grass)
./build.sh

# Run in a terminal at least 128x33 characters
./touch-grass
```

**Controls:** Arrow keys or WASD (move), Z/Space/Enter (A button), X/Esc (B button), Q (quit)

## Architecture

TouchGrass is a tile-based exploration game for the terminal, written in C11
with no dependencies beyond a C compiler and POSIX. The game renders a 128x64
monochrome framebuffer as Unicode half-block characters (2 pixels per cell,
so a 128x32 character display).

### File Structure

- `src/main.c` - Entry point; fixed-timestep loop (~30 FPS) calling `game_setup()` / `game_loop()`
- `src/platform.h` - Platform API (display, input, sound, timing, quit)
- `src/platform_terminal.c` - Terminal implementation: ANSI half-block rendering, termios raw-mode input, monotonic clock; sound is a no-op
- `src/game/game.h` - Game state machine, rendering, and input handling
- `src/game/config.h` - Game constants (map size, hunger, growth timers)
- `src/game/terrain.h` - Procedural map generation (rivers, dirt patches, trees), player movement
- `src/game/sprites.h` - 8x8 tile sprites and tile type constants
- `src/game/inventory.h` - Item types and 12-slot inventory
- `src/game/building.h` - Building interiors (stove, chest, computer)
- `src/game/creatures.h`, `src/game/chunks.h`, `src/game/progression.h`, `src/game/dialog.h` - Creatures, world chunks, progression flags, dialog overlay
- `src/game/save.h` - Save system: 6 slots as binary files in `~/.touchgrass/saves/`
- `src/game/t9_dict.h`, `src/game/t9_input.h` - T9 text entry for save names

### Game State

The game uses a state machine (`GameState` in `game.h`): splash, menu, world,
tile view, inventory, buildings, chests, save/load flows, dialog, and more.
Player position is tracked separately from the map via `tg_playerX`,
`tg_playerY`, and `tg_underPlayer` (stores the tile type under the player).

### Platform API

```c
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

// Sound (no-op in the terminal implementation)
platform_beep(freq, duration_ms)
platform_play_melody(notes, durations, len)
```

### Terminal Input Caveat

Terminals never report key-up events, so a button counts as "held" for 150ms
after its most recent key event (`KEY_HOLD_MS` in `platform_terminal.c`); OS
key repeat keeps it held. Edge detection (`platform_button_pressed`) works
normally on top of this.
