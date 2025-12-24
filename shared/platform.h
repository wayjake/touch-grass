/*
 * Platform Abstraction Layer
 *
 * Unified API for display, input, sound, and timing that can be implemented
 * for different targets (ESP32 hardware, web browser via Emscripten, etc.)
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Constants
// ============================================================================

#define PLATFORM_SCREEN_WIDTH   128
#define PLATFORM_SCREEN_HEIGHT  64
#define PLATFORM_TILE_SIZE      8

// ============================================================================
// Button Definitions
// ============================================================================

typedef enum {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_A,
    BTN_B,
    BTN_COUNT
} PlatformButton;

// ============================================================================
// Initialization
// ============================================================================

// Initialize all platform subsystems (display, input, audio)
void platform_init(void);

// ============================================================================
// Display - Core
// ============================================================================

// Clear the screen to black
void platform_clear_screen(void);

// Flush the framebuffer to the display
void platform_render(void);

// ============================================================================
// Display - Primitives
// ============================================================================

// Set a single pixel (color: true = white, false = black)
void platform_set_pixel(int x, int y, bool color);

// Draw a horizontal line
void platform_draw_hline(int x, int y, int w, bool color);

// Draw a vertical line
void platform_draw_vline(int x, int y, int h, bool color);

// Draw a line between two points
void platform_draw_line(int x0, int y0, int x1, int y1, bool color);

// Draw a rectangle outline
void platform_draw_rect(int x, int y, int w, int h, bool color);

// Draw a filled rectangle
void platform_fill_rect(int x, int y, int w, int h, bool color);

// ============================================================================
// Display - Text
// ============================================================================

// Set text cursor position
void platform_set_cursor(int x, int y);

// Set text size (1 = 6x8, 2 = 12x16, etc.)
void platform_set_text_size(uint8_t size);

// Print a string at current cursor position
void platform_print(const char* str);

// Print a single character
void platform_print_char(char c);

// Print an integer
void platform_print_int(int value);

// ============================================================================
// Display - Sprites/Tiles
// ============================================================================

// Draw an 8x8 tile sprite at tile grid coordinates
// tileData: 8 bytes, one per row, MSB-first bit ordering, stored in PROGMEM
void platform_draw_tile(uint8_t tileX, uint8_t tileY, const uint8_t* tileData);

// Draw an 8x8 sprite at pixel coordinates with scaling
void platform_draw_sprite_scaled(int x, int y, const uint8_t* spriteData, int scale);

// Draw XBM bitmap (LSB-first bit ordering) - used for some assets
void platform_draw_xbm(int x, int y, int w, int h, const uint8_t* bitmap, bool color);

// ============================================================================
// Input
// ============================================================================

// Update button states (call once per frame at start of loop)
void platform_input_update(void);

// Returns true on rising edge (button just pressed this frame)
bool platform_button_pressed(PlatformButton btn);

// Returns true on falling edge (button just released this frame)
bool platform_button_released(PlatformButton btn);

// Returns true while button is held down
bool platform_button_held(PlatformButton btn);

// Returns how long button has been held in milliseconds (0 if not held)
unsigned long platform_button_held_ms(PlatformButton btn);

// ============================================================================
// Input - Convenience
// ============================================================================

static inline bool platform_dpad_up(void)    { return platform_button_held(BTN_UP); }
static inline bool platform_dpad_down(void)  { return platform_button_held(BTN_DOWN); }
static inline bool platform_dpad_left(void)  { return platform_button_held(BTN_LEFT); }
static inline bool platform_dpad_right(void) { return platform_button_held(BTN_RIGHT); }

static inline bool platform_dpad_up_pressed(void)    { return platform_button_pressed(BTN_UP); }
static inline bool platform_dpad_down_pressed(void)  { return platform_button_pressed(BTN_DOWN); }
static inline bool platform_dpad_left_pressed(void)  { return platform_button_pressed(BTN_LEFT); }
static inline bool platform_dpad_right_pressed(void) { return platform_button_pressed(BTN_RIGHT); }

// ============================================================================
// Sound
// ============================================================================

// Play a simple beep/tone
void platform_beep(int freq, int duration_ms);

// Play a melody (blocking) - notes and durations are in PROGMEM
void platform_play_melody(const int* notes, const int* durations, int length);

// Set background melody for non-blocking looping playback
void platform_set_melody(const int* notes, const int* durations, int length);

// Update background melody (call every frame)
void platform_update_melody(void);

// Stop background melody
void platform_stop_melody(void);

// ============================================================================
// LED (optional - no-op on platforms without RGB LED)
// ============================================================================

void platform_led_set(uint8_t r, uint8_t g, uint8_t b);
void platform_led_off(void);

// ============================================================================
// System Controls (volume, brightness)
// ============================================================================

// Get/set sound volume (0-100, 0 = muted)
uint8_t platform_get_volume(void);
void platform_set_volume(uint8_t volume);

// Get/set display brightness (0-100)
uint8_t platform_get_brightness(void);
void platform_set_brightness(uint8_t brightness);

// ============================================================================
// Timing
// ============================================================================

// Get milliseconds since startup
unsigned long platform_millis(void);

// Delay for specified milliseconds (avoid in game loop - use timing checks)
void platform_delay(int ms);

// ============================================================================
// PROGMEM Access (cross-platform)
// ============================================================================

#ifdef ARDUINO
    // Arduino platforms (ESP32, AVR, etc.) already have pgm_read_* via Arduino.h
    #define platform_pgm_read_byte(addr) pgm_read_byte(addr)
    #define platform_pgm_read_word(addr) pgm_read_word(addr)
    #define PLATFORM_PROGMEM PROGMEM
#else
    // On web/desktop, PROGMEM is just regular memory
    #define platform_pgm_read_byte(addr) (*(const uint8_t*)(addr))
    #define platform_pgm_read_word(addr) (*(const uint16_t*)(addr))
    #define PLATFORM_PROGMEM
#endif

#ifdef __cplusplus
}
#endif

#endif // PLATFORM_H
