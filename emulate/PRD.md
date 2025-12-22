# TouchGrass Browser Emulator PRD

## Overview

Port the TouchGrass ESP32 game to run in the browser using Emscripten (C → WebAssembly) with a React frontend for the visual and audio interface.

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Game Logic                           │
│    (terrain.h, sprites.h, inventory.h, building.h)      │
├─────────────────────────────────────────────────────────┤
│              Platform Abstraction Layer                 │
│                    (platform.h)                         │
├───────────────────────┬─────────────────────────────────┤
│    ESP32 Backend      │        Web Backend              │
│    (platform_esp32.h) │        (platform_web.c)         │
│                       │                                 │
│  - Adafruit_GFX       │  - Framebuffer → JS export      │
│  - tone()             │  - Emscripten audio bindings    │
│  - GPIO digitalRead   │  - Keyboard event callbacks     │
└───────────────────────┴─────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────┐
│                 React Frontend                          │
│                                                         │
│  - Canvas renderer (reads WASM framebuffer)             │
│  - Web Audio API oscillator (square wave)               │
│  - Keyboard listener → WASM input state                 │
│  - Game loop via requestAnimationFrame                  │
└─────────────────────────────────────────────────────────┘
```

## Current C API Surface

### Display (graphics.h)
```c
// Currently uses Adafruit_GFX display object directly
display.drawPixel(x, y, color)
display.fillScreen(SH110X_BLACK)
display.display()  // flush to OLED

void drawXbm(x, y, w, h, bitmap, color)  // XBM bitmap rendering
void drawTile(tileX, tileY, tileData)    // 8x8 tile at grid position
```

### Input (hardware.h)
```c
void button_update()                    // Poll once per frame
bool button_pressed(Button btn)         // Rising edge
bool button_released(Button btn)        // Falling edge
bool button_held(Button btn)            // Currently down
unsigned long button_held_ms(Button)    // Hold duration

// D-pad convenience
bool dpad_up(), dpad_down(), dpad_left(), dpad_right()
bool dpad_up_pressed(), dpad_down_pressed(), etc.
```

### Sound (sound.h)
```c
void playBeep(int freq, int durationMs)
void playMelody(notes[], durations[], length)           // Blocking
void setBackgroundMelodyCustom(notes[], durations[], len) // Non-blocking loop
void updateBackgroundMelody()                           // Tick each frame
void stopBackgroundMelody()
```

### Constants (config.h)
```c
SCREEN_WIDTH   128
SCREEN_HEIGHT  64
TILE_SIZE      8
MAP_WIDTH      16  // tiles
MAP_HEIGHT     8   // tiles
```

## Platform Abstraction API

New unified API that both ESP32 and Web backends implement:

```c
// platform.h

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

// Display
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

void platform_init(void);
void platform_clear_screen(void);
void platform_set_pixel(int x, int y, bool on);
void platform_render(void);  // Flush framebuffer to display

// Convenience drawing (implemented in terms of set_pixel)
void platform_draw_tile(int tileX, int tileY, const uint8_t* tileData);

// Input
typedef enum {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,
    BTN_B,
    BTN_COUNT
} Button;

void platform_input_update(void);
bool platform_button_pressed(Button btn);
bool platform_button_released(Button btn);
bool platform_button_held(Button btn);

// Sound
void platform_beep(int freq, int duration_ms);
void platform_set_melody(const int* notes, const int* durations, int length);
void platform_update_melody(void);
void platform_stop_melody(void);

// Frame timing
void platform_delay(int ms);
unsigned long platform_millis(void);

#endif
```

## Web Backend Implementation

### Framebuffer Strategy

```c
// platform_web.c
#include <emscripten.h>
#include "platform.h"

// 128x64 monochrome = 1024 bytes (1 bit per pixel, packed)
static uint8_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT / 8];

// Export framebuffer pointer to JS
EMSCRIPTEN_KEEPALIVE
uint8_t* platform_get_framebuffer(void) {
    return framebuffer;
}

EMSCRIPTEN_KEEPALIVE
int platform_get_framebuffer_size(void) {
    return sizeof(framebuffer);
}

void platform_set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    int byteIndex = (y * SCREEN_WIDTH + x) / 8;
    int bitIndex = (y * SCREEN_WIDTH + x) % 8;
    if (on) {
        framebuffer[byteIndex] |= (1 << bitIndex);
    } else {
        framebuffer[byteIndex] &= ~(1 << bitIndex);
    }
}

void platform_render(void) {
    // Notify JS that frame is ready
    EM_ASM({ if (window.onFrameReady) window.onFrameReady(); });
}
```

### Input via JS Callbacks

```c
// Input state set by JS keyboard handlers
static bool button_current[BTN_COUNT] = {0};
static bool button_previous[BTN_COUNT] = {0};

EMSCRIPTEN_KEEPALIVE
void platform_set_button(int btn, bool pressed) {
    if (btn >= 0 && btn < BTN_COUNT) {
        button_current[btn] = pressed;
    }
}

void platform_input_update(void) {
    for (int i = 0; i < BTN_COUNT; i++) {
        button_previous[i] = button_current[i];
    }
}

bool platform_button_pressed(Button btn) {
    return button_current[btn] && !button_previous[btn];
}

bool platform_button_held(Button btn) {
    return button_current[btn];
}
```

### Sound via Web Audio

```c
void platform_beep(int freq, int duration_ms) {
    EM_ASM({
        if (window.playBeep) window.playBeep($0, $1);
    }, freq, duration_ms);
}
```

## React Integration

### File Structure

```
emulate/
├── app/
│   ├── routes/
│   │   ├── home.tsx
│   │   └── emulator.tsx          # New emulator page
│   ├── components/
│   │   └── Emulator/
│   │       ├── Emulator.tsx      # Main emulator component
│   │       ├── Display.tsx       # Canvas renderer
│   │       ├── Controls.tsx      # On-screen buttons (mobile)
│   │       └── useGameLoop.ts    # WASM loading & game loop hook
│   └── routes.ts                 # Add emulator route
├── public/
│   └── wasm/
│       ├── game.wasm             # Compiled game
│       └── game.js               # Emscripten glue
├── wasm/                         # C source for web build
│   ├── platform.h
│   ├── platform_web.c
│   └── main.c                    # Entry point, includes game logic
└── build-wasm.sh                 # Emscripten build script
```

### Canvas Renderer Component

```tsx
// app/components/Emulator/Display.tsx
import { useEffect, useRef } from 'react';

interface DisplayProps {
  wasmModule: any;
  scale?: number;
}

export function Display({ wasmModule, scale = 4 }: DisplayProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas || !wasmModule) return;

    const ctx = canvas.getContext('2d')!;
    const imageData = ctx.createImageData(128, 64);

    // Called by WASM when frame is ready
    window.onFrameReady = () => {
      const fbPtr = wasmModule._platform_get_framebuffer();
      const fb = new Uint8Array(wasmModule.HEAPU8.buffer, fbPtr, 1024);

      // Convert 1-bit packed to RGBA
      for (let i = 0; i < 128 * 64; i++) {
        const byteIndex = Math.floor(i / 8);
        const bitIndex = i % 8;
        const on = (fb[byteIndex] >> bitIndex) & 1;
        const color = on ? 255 : 0;

        imageData.data[i * 4 + 0] = color;     // R
        imageData.data[i * 4 + 1] = color;     // G
        imageData.data[i * 4 + 2] = color;     // B
        imageData.data[i * 4 + 3] = 255;       // A
      }

      ctx.putImageData(imageData, 0, 0);
    };

    return () => { window.onFrameReady = undefined; };
  }, [wasmModule]);

  return (
    <canvas
      ref={canvasRef}
      width={128}
      height={64}
      className="bg-black"
      style={{
        width: 128 * scale,
        height: 64 * scale,
        imageRendering: 'pixelated',
      }}
    />
  );
}
```

### Keyboard Input Hook

```tsx
// app/components/Emulator/useKeyboardInput.ts
import { useEffect } from 'react';

const KEY_MAP: Record<string, number> = {
  'ArrowUp': 0,    // BTN_UP
  'ArrowDown': 1,  // BTN_DOWN
  'ArrowLeft': 2,  // BTN_LEFT
  'ArrowRight': 3, // BTN_RIGHT
  'KeyZ': 4,       // BTN_A
  'KeyX': 5,       // BTN_B
  'Space': 4,      // BTN_A (alternative)
};

export function useKeyboardInput(wasmModule: any) {
  useEffect(() => {
    if (!wasmModule) return;

    const handleKeyDown = (e: KeyboardEvent) => {
      const btn = KEY_MAP[e.code];
      if (btn !== undefined) {
        e.preventDefault();
        wasmModule._platform_set_button(btn, true);
      }
    };

    const handleKeyUp = (e: KeyboardEvent) => {
      const btn = KEY_MAP[e.code];
      if (btn !== undefined) {
        wasmModule._platform_set_button(btn, false);
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [wasmModule]);
}
```

### Web Audio Sound

```tsx
// app/components/Emulator/useAudio.ts
import { useEffect, useRef } from 'react';

export function useAudio() {
  const audioCtxRef = useRef<AudioContext | null>(null);

  useEffect(() => {
    // Create audio context on first user interaction
    const initAudio = () => {
      if (!audioCtxRef.current) {
        audioCtxRef.current = new AudioContext();
      }
    };

    window.addEventListener('click', initAudio, { once: true });
    window.addEventListener('keydown', initAudio, { once: true });

    // Expose beep function to WASM
    window.playBeep = (freq: number, durationMs: number) => {
      const ctx = audioCtxRef.current;
      if (!ctx || freq === 0) return;

      const oscillator = ctx.createOscillator();
      const gain = ctx.createGain();

      oscillator.type = 'square';  // Match buzzer aesthetic
      oscillator.frequency.value = freq;
      gain.gain.value = 0.1;  // Don't blow out speakers

      oscillator.connect(gain);
      gain.connect(ctx.destination);

      oscillator.start();
      oscillator.stop(ctx.currentTime + durationMs / 1000);
    };

    return () => {
      window.playBeep = undefined;
      audioCtxRef.current?.close();
    };
  }, []);
}
```

## Build Pipeline

### Emscripten Build Script

```bash
#!/bin/bash
# build-wasm.sh

set -e

GAME_DIR="../"
WASM_DIR="./wasm"
OUTPUT_DIR="./public/wasm"

mkdir -p "$OUTPUT_DIR"

emcc \
  "$WASM_DIR/main.c" \
  "$WASM_DIR/platform_web.c" \
  -I"$GAME_DIR/shared" \
  -I"$GAME_DIR/touch_grass" \
  -o "$OUTPUT_DIR/game.js" \
  -s WASM=1 \
  -s EXPORTED_FUNCTIONS='["_main", "_platform_get_framebuffer", "_platform_set_button", "_game_tick"]' \
  -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
  -s MODULARIZE=1 \
  -s EXPORT_NAME="createGameModule" \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s ENVIRONMENT='web' \
  -O2

echo "Build complete: $OUTPUT_DIR/game.js, $OUTPUT_DIR/game.wasm"
```

### Main Entry Point

```c
// wasm/main.c
#include "platform.h"

// Include game logic headers (they use platform_* functions now)
// This requires refactoring the game to use platform abstraction

extern void game_init(void);
extern void game_tick(void);  // Single frame update

int main() {
    platform_init();
    game_init();

    // For Emscripten, we don't use a while loop
    // Instead, JS calls game_tick via requestAnimationFrame
    #ifdef __EMSCRIPTEN__
    // Main returns, JS drives the loop
    #else
    while (1) {
        game_tick();
        platform_delay(16);  // ~60fps
    }
    #endif

    return 0;
}

// Exported for JS to call each frame
EMSCRIPTEN_KEEPALIVE
void game_tick(void) {
    platform_input_update();
    // ... game logic from TouchGrass.ino loop() ...
    platform_update_melody();
    platform_render();
}
```

## Implementation Steps

### Phase 1: Platform Abstraction Layer
1. Create `platform.h` with unified API
2. Create `platform_esp32.h` that wraps existing Adafruit/Arduino calls
3. Refactor `TouchGrass.ino` to use `platform_*` functions
4. Verify game still works on ESP32

### Phase 2: Web Backend
1. Create `platform_web.c` implementing all platform functions
2. Create `main.c` entry point with `game_tick()` export
3. Set up Emscripten build script
4. Test basic compilation

### Phase 3: React Integration
1. Add `/emulator` route to routes.ts
2. Create Emulator component with Display, Controls
3. Implement WASM loading and game loop
4. Add keyboard input handling
5. Add Web Audio for sound

### Phase 4: Polish
1. Add on-screen touch controls for mobile
2. Add save/load state (localStorage)
3. Add fullscreen mode
4. Style to match retro aesthetic
5. Add loading screen while WASM compiles

## Key Decisions

### Why Framebuffer Export (not direct canvas calls from C)?
- Cleaner separation: C doesn't need to know about canvas
- React controls render timing via requestAnimationFrame
- Easier to add effects/scaling on JS side
- Matches how real hardware works (memory-mapped display)

### Why Square Wave Oscillator?
- ESP32 buzzer produces square waves via `tone()`
- Authentic retro sound aesthetic
- Simple and efficient

### Why Keyboard + Touch Controls?
- Desktop: Arrow keys + Z/X feels natural for retro games
- Mobile: On-screen D-pad required (no keyboard)
- Touch controls can be optional overlay

## Notes on Game Logic Porting

The current `TouchGrass.ino` uses Arduino's `loop()` function which runs continuously. For Emscripten:

1. Extract loop body into `game_tick()` function
2. Remove any `delay()` calls (JS handles timing)
3. Replace all `display.*` calls with `platform_*` equivalents
4. Replace all `button_*` calls with `platform_button_*`
5. Replace all sound calls with `platform_*` equivalents

The `PROGMEM` sprite data needs special handling:
- Option A: Convert to regular arrays (wastes RAM but simple)
- Option B: Keep as-is, `pgm_read_byte()` becomes no-op macro on web
