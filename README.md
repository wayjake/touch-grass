# TouchGrass

A tile-based exploration game that runs right in your terminal.

![Early Prototype](early-prototype.png)

Explore a procedurally generated world, gather resources, build, cook, catch
creatures, and try to survive. The whole game renders on a 128x64 monochrome
framebuffer drawn with Unicode half-block characters.

## Requirements

- macOS or Linux
- A C compiler (clang or gcc)
- A terminal at least 128x33 characters

## Build & Run

```bash
./build.sh
./touch-grass
```

## Controls

| Key | Action |
|-----|--------|
| Arrow keys / WASD | Move |
| Z / Space / Enter | A button (interact/select) |
| X / Esc | B button (back/cancel) |
| Q / Ctrl+C | Quit |

Saves are stored in `~/.touchgrass/saves/` (6 slots).

## Project Structure

- `src/main.c` - Entry point and fixed-timestep game loop
- `src/platform.h` - Platform API (display, input, sound, timing)
- `src/platform_terminal.c` - Terminal implementation (ANSI rendering, termios input)
- `src/game/` - Game logic: state machine, terrain generation, inventory, buildings, creatures, dialog, T9 text input, saves
