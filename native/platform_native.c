/*
 * Platform Implementation for Native Terminal (macOS / Linux)
 *
 * Implements the platform abstraction API as a raw POSIX executable.
 * - Display: 128x64 monochrome framebuffer rendered with Unicode half-block
 *   characters (2 pixels per terminal cell -> 128x32 characters)
 * - Input: termios raw mode; arrow keys / WASD for d-pad, Z/Space = A,
 *   X/Esc = B, Q or Ctrl+C to quit
 * - Sound: no-op (terminal has no tone generator)
 * - Timing: clock_gettime(CLOCK_MONOTONIC)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include "../shared/platform.h"

// How long a key event keeps its button "held" (bridges terminal key-repeat)
#define KEY_HOLD_MS 150

// ============================================================================
// Framebuffer (128x64 monochrome = 1024 bytes)
// ============================================================================

static uint8_t framebuffer[PLATFORM_SCREEN_WIDTH * PLATFORM_SCREEN_HEIGHT / 8];

// Text cursor state
static int _cursorX = 0;
static int _cursorY = 0;
static uint8_t _textSize = 1;

// Timing
static struct timespec _startTime;

// Quit flag (set by Q / Ctrl+C)
static volatile sig_atomic_t _quitRequested = 0;

bool platform_native_should_quit(void) {
    return _quitRequested != 0;
}

// ============================================================================
// Simple 6x8 Bitmap Font (ASCII 32-126)
// ============================================================================

// Each character is 6 pixels wide, 8 pixels tall
// Stored as 6 bytes per character (one byte per column)
static const uint8_t font_6x8[] = {
    // Space (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x00, 0x00, 0x5F, 0x00, 0x00, 0x00,
    // " (34)
    0x00, 0x07, 0x00, 0x07, 0x00, 0x00,
    // # (35)
    0x14, 0x7F, 0x14, 0x7F, 0x14, 0x00,
    // $ (36)
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00,
    // % (37)
    0x23, 0x13, 0x08, 0x64, 0x62, 0x00,
    // & (38)
    0x36, 0x49, 0x55, 0x22, 0x50, 0x00,
    // ' (39)
    0x00, 0x05, 0x03, 0x00, 0x00, 0x00,
    // ( (40)
    0x00, 0x1C, 0x22, 0x41, 0x00, 0x00,
    // ) (41)
    0x00, 0x41, 0x22, 0x1C, 0x00, 0x00,
    // * (42)
    0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00,
    // + (43)
    0x08, 0x08, 0x3E, 0x08, 0x08, 0x00,
    // , (44)
    0x00, 0x50, 0x30, 0x00, 0x00, 0x00,
    // - (45)
    0x08, 0x08, 0x08, 0x08, 0x08, 0x00,
    // . (46)
    0x00, 0x60, 0x60, 0x00, 0x00, 0x00,
    // / (47)
    0x20, 0x10, 0x08, 0x04, 0x02, 0x00,
    // 0 (48)
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00,
    // 1 (49)
    0x00, 0x42, 0x7F, 0x40, 0x00, 0x00,
    // 2 (50)
    0x42, 0x61, 0x51, 0x49, 0x46, 0x00,
    // 3 (51)
    0x21, 0x41, 0x45, 0x4B, 0x31, 0x00,
    // 4 (52)
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x00,
    // 5 (53)
    0x27, 0x45, 0x45, 0x45, 0x39, 0x00,
    // 6 (54)
    0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00,
    // 7 (55)
    0x01, 0x71, 0x09, 0x05, 0x03, 0x00,
    // 8 (56)
    0x36, 0x49, 0x49, 0x49, 0x36, 0x00,
    // 9 (57)
    0x06, 0x49, 0x49, 0x29, 0x1E, 0x00,
    // : (58)
    0x00, 0x36, 0x36, 0x00, 0x00, 0x00,
    // ; (59)
    0x00, 0x56, 0x36, 0x00, 0x00, 0x00,
    // < (60)
    0x00, 0x08, 0x14, 0x22, 0x41, 0x00,
    // = (61)
    0x14, 0x14, 0x14, 0x14, 0x14, 0x00,
    // > (62)
    0x41, 0x22, 0x14, 0x08, 0x00, 0x00,
    // ? (63)
    0x02, 0x01, 0x51, 0x09, 0x06, 0x00,
    // @ (64)
    0x32, 0x49, 0x79, 0x41, 0x3E, 0x00,
    // A (65)
    0x7E, 0x11, 0x11, 0x11, 0x7E, 0x00,
    // B (66)
    0x7F, 0x49, 0x49, 0x49, 0x36, 0x00,
    // C (67)
    0x3E, 0x41, 0x41, 0x41, 0x22, 0x00,
    // D (68)
    0x7F, 0x41, 0x41, 0x22, 0x1C, 0x00,
    // E (69)
    0x7F, 0x49, 0x49, 0x49, 0x41, 0x00,
    // F (70)
    0x7F, 0x09, 0x09, 0x01, 0x01, 0x00,
    // G (71)
    0x3E, 0x41, 0x41, 0x51, 0x32, 0x00,
    // H (72)
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00,
    // I (73)
    0x00, 0x41, 0x7F, 0x41, 0x00, 0x00,
    // J (74)
    0x20, 0x40, 0x41, 0x3F, 0x01, 0x00,
    // K (75)
    0x7F, 0x08, 0x14, 0x22, 0x41, 0x00,
    // L (76)
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x00,
    // M (77)
    0x7F, 0x02, 0x04, 0x02, 0x7F, 0x00,
    // N (78)
    0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00,
    // O (79)
    0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00,
    // P (80)
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x00,
    // Q (81)
    0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00,
    // R (82)
    0x7F, 0x09, 0x19, 0x29, 0x46, 0x00,
    // S (83)
    0x46, 0x49, 0x49, 0x49, 0x31, 0x00,
    // T (84)
    0x01, 0x01, 0x7F, 0x01, 0x01, 0x00,
    // U (85)
    0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00,
    // V (86)
    0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00,
    // W (87)
    0x7F, 0x20, 0x18, 0x20, 0x7F, 0x00,
    // X (88)
    0x63, 0x14, 0x08, 0x14, 0x63, 0x00,
    // Y (89)
    0x03, 0x04, 0x78, 0x04, 0x03, 0x00,
    // Z (90)
    0x61, 0x51, 0x49, 0x45, 0x43, 0x00,
    // [ (91)
    0x00, 0x00, 0x7F, 0x41, 0x41, 0x00,
    // \ (92)
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00,
    // ] (93)
    0x41, 0x41, 0x7F, 0x00, 0x00, 0x00,
    // ^ (94)
    0x04, 0x02, 0x01, 0x02, 0x04, 0x00,
    // _ (95)
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00,
    // ` (96)
    0x00, 0x01, 0x02, 0x04, 0x00, 0x00,
    // a (97)
    0x20, 0x54, 0x54, 0x54, 0x78, 0x00,
    // b (98)
    0x7F, 0x48, 0x44, 0x44, 0x38, 0x00,
    // c (99)
    0x38, 0x44, 0x44, 0x44, 0x20, 0x00,
    // d (100)
    0x38, 0x44, 0x44, 0x48, 0x7F, 0x00,
    // e (101)
    0x38, 0x54, 0x54, 0x54, 0x18, 0x00,
    // f (102)
    0x08, 0x7E, 0x09, 0x01, 0x02, 0x00,
    // g (103)
    0x08, 0x14, 0x54, 0x54, 0x3C, 0x00,
    // h (104)
    0x7F, 0x08, 0x04, 0x04, 0x78, 0x00,
    // i (105)
    0x00, 0x44, 0x7D, 0x40, 0x00, 0x00,
    // j (106)
    0x20, 0x40, 0x44, 0x3D, 0x00, 0x00,
    // k (107)
    0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,
    // l (108)
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x00,
    // m (109)
    0x7C, 0x04, 0x18, 0x04, 0x78, 0x00,
    // n (110)
    0x7C, 0x08, 0x04, 0x04, 0x78, 0x00,
    // o (111)
    0x38, 0x44, 0x44, 0x44, 0x38, 0x00,
    // p (112)
    0x7C, 0x14, 0x14, 0x14, 0x08, 0x00,
    // q (113)
    0x08, 0x14, 0x14, 0x18, 0x7C, 0x00,
    // r (114)
    0x7C, 0x08, 0x04, 0x04, 0x08, 0x00,
    // s (115)
    0x48, 0x54, 0x54, 0x54, 0x20, 0x00,
    // t (116)
    0x04, 0x3F, 0x44, 0x40, 0x20, 0x00,
    // u (117)
    0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00,
    // v (118)
    0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00,
    // w (119)
    0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00,
    // x (120)
    0x44, 0x28, 0x10, 0x28, 0x44, 0x00,
    // y (121)
    0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00,
    // z (122)
    0x44, 0x64, 0x54, 0x4C, 0x44, 0x00,
    // { (123)
    0x00, 0x08, 0x36, 0x41, 0x00, 0x00,
    // | (124)
    0x00, 0x00, 0x7F, 0x00, 0x00, 0x00,
    // } (125)
    0x00, 0x41, 0x36, 0x08, 0x00, 0x00,
    // ~ (126)
    0x08, 0x08, 0x2A, 0x1C, 0x08, 0x00,
};

// ============================================================================
// Terminal Setup / Teardown
// ============================================================================

static struct termios _origTermios;
static bool _termiosSaved = false;
static int _origStdinFlags = -1;

static void terminal_restore(void) {
    if (_termiosSaved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &_origTermios);
    }
    if (_origStdinFlags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, _origStdinFlags);
    }
    // Show cursor, leave alternate screen
    const char* restore = "\x1b[?25h\x1b[?1049l";
    ssize_t n = write(STDOUT_FILENO, restore, strlen(restore));
    (void)n;
}

static void handle_signal(int sig) {
    (void)sig;
    _quitRequested = 1;
}

static void terminal_setup(void) {
    if (tcgetattr(STDIN_FILENO, &_origTermios) == 0) {
        _termiosSaved = true;
        struct termios raw = _origTermios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_cc[VMIN] = 0;   // Non-blocking reads
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    // Non-blocking stdin also covers non-tty input (pipes, automation);
    // restored at exit since fd flags are shared with the parent shell
    _origStdinFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (_origStdinFlags != -1) {
        fcntl(STDIN_FILENO, F_SETFL, _origStdinFlags | O_NONBLOCK);
    }
    atexit(terminal_restore);
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Enter alternate screen, clear, hide cursor
    const char* setup = "\x1b[?1049h\x1b[2J\x1b[?25l";
    ssize_t n = write(STDOUT_FILENO, setup, strlen(setup));
    (void)n;
}

// ============================================================================
// Input State
// ============================================================================

static bool _buttonCurrent[BTN_COUNT] = {0};
static bool _buttonPrevious[BTN_COUNT] = {0};
static bool _buttonJustPressed[BTN_COUNT] = {0};
static bool _buttonJustReleased[BTN_COUNT] = {0};
static unsigned long _buttonPressStart[BTN_COUNT] = {0};
// Last time a key event arrived for each button; a button counts as "held"
// for KEY_HOLD_MS after its most recent key event (terminals have no key-up)
static unsigned long _buttonLastEvent[BTN_COUNT] = {0};
static bool _buttonEverPressed[BTN_COUNT] = {0};

static void register_key(PlatformButton btn) {
    _buttonLastEvent[btn] = platform_millis();
    _buttonEverPressed[btn] = true;
}

static void poll_terminal_input(void) {
    unsigned char buf[64];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));

    for (ssize_t i = 0; i < n; i++) {
        unsigned char c = buf[i];

        // Escape sequences (arrow keys) vs bare ESC
        if (c == 0x1b) {
            if (i + 2 < n && buf[i + 1] == '[') {
                switch (buf[i + 2]) {
                    case 'A': register_key(BTN_UP); break;
                    case 'B': register_key(BTN_DOWN); break;
                    case 'C': register_key(BTN_RIGHT); break;
                    case 'D': register_key(BTN_LEFT); break;
                }
                i += 2;
            } else {
                register_key(BTN_B);  // Bare ESC = B button
            }
            continue;
        }

        switch (c) {
            case 'w': case 'W': register_key(BTN_UP); break;
            case 's': case 'S': register_key(BTN_DOWN); break;
            case 'a': case 'A': register_key(BTN_LEFT); break;
            case 'd': case 'D': register_key(BTN_RIGHT); break;
            case 'z': case 'Z': case ' ': case '\n': case '\r':
                register_key(BTN_A); break;
            case 'x': case 'X': register_key(BTN_B); break;
            case 'q': case 'Q': case 0x03:  // Q or Ctrl+C
                _quitRequested = 1; break;
        }
    }
}

// ============================================================================
// Initialization
// ============================================================================

void platform_init(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
    memset(_buttonCurrent, 0, sizeof(_buttonCurrent));
    memset(_buttonPrevious, 0, sizeof(_buttonPrevious));
    _cursorX = 0;
    _cursorY = 0;
    _textSize = 1;

    clock_gettime(CLOCK_MONOTONIC, &_startTime);
    terminal_setup();
}

// ============================================================================
// Display - Core
// ============================================================================

void platform_clear_screen(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
}

static bool get_pixel(int x, int y) {
    if (x < 0 || x >= PLATFORM_SCREEN_WIDTH || y < 0 || y >= PLATFORM_SCREEN_HEIGHT) return false;

    int byteIndex = (y * PLATFORM_SCREEN_WIDTH + x) / 8;
    int bitIndex = (y * PLATFORM_SCREEN_WIDTH + x) % 8;

    return (framebuffer[byteIndex] >> bitIndex) & 1;
}

void platform_render(void) {
    // Each terminal cell shows 2 vertically stacked pixels via half-blocks:
    // upper='▀' (E2 96 80), lower='▄' (E2 96 84), both='█' (E2 96 88)
    // Frame: 32 rows x 128 cols, worst case 3 bytes/char + control codes
    static char out[32 * (128 * 3 + 8) + 64];
    size_t pos = 0;

    // Home cursor
    memcpy(out + pos, "\x1b[H", 3);
    pos += 3;

    for (int row = 0; row < PLATFORM_SCREEN_HEIGHT / 2; row++) {
        for (int x = 0; x < PLATFORM_SCREEN_WIDTH; x++) {
            bool top = get_pixel(x, row * 2);
            bool bottom = get_pixel(x, row * 2 + 1);

            if (top && bottom) {
                memcpy(out + pos, "\xe2\x96\x88", 3); pos += 3;  // █
            } else if (top) {
                memcpy(out + pos, "\xe2\x96\x80", 3); pos += 3;  // ▀
            } else if (bottom) {
                memcpy(out + pos, "\xe2\x96\x84", 3); pos += 3;  // ▄
            } else {
                out[pos++] = ' ';
            }
        }
        memcpy(out + pos, "\x1b[K\r\n", 5);  // Clear to EOL, next line
        pos += 5;
    }

    ssize_t n = write(STDOUT_FILENO, out, pos);
    (void)n;
}

// ============================================================================
// Display - Primitives
// ============================================================================

void platform_set_pixel(int x, int y, bool color) {
    if (x < 0 || x >= PLATFORM_SCREEN_WIDTH || y < 0 || y >= PLATFORM_SCREEN_HEIGHT) return;

    int byteIndex = (y * PLATFORM_SCREEN_WIDTH + x) / 8;
    int bitIndex = (y * PLATFORM_SCREEN_WIDTH + x) % 8;

    if (color) {
        framebuffer[byteIndex] |= (1 << bitIndex);
    } else {
        framebuffer[byteIndex] &= ~(1 << bitIndex);
    }
}

void platform_draw_hline(int x, int y, int w, bool color) {
    for (int i = 0; i < w; i++) {
        platform_set_pixel(x + i, y, color);
    }
}

void platform_draw_vline(int x, int y, int h, bool color) {
    for (int i = 0; i < h; i++) {
        platform_set_pixel(x, y + i, color);
    }
}

void platform_draw_line(int x0, int y0, int x1, int y1, bool color) {
    // Bresenham's line algorithm
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        platform_set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void platform_draw_rect(int x, int y, int w, int h, bool color) {
    platform_draw_hline(x, y, w, color);
    platform_draw_hline(x, y + h - 1, w, color);
    platform_draw_vline(x, y, h, color);
    platform_draw_vline(x + w - 1, y, h, color);
}

void platform_fill_rect(int x, int y, int w, int h, bool color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            platform_set_pixel(x + i, y + j, color);
        }
    }
}

// ============================================================================
// Display - Text
// ============================================================================

void platform_set_cursor(int x, int y) {
    _cursorX = x;
    _cursorY = y;
}

void platform_set_text_size(uint8_t size) {
    _textSize = size > 0 ? size : 1;
}

static void draw_char(int x, int y, char c, uint8_t size) {
    if (c < 32 || c > 126) c = '?';

    int idx = (c - 32) * 6;

    for (int col = 0; col < 6; col++) {
        uint8_t line = font_6x8[idx + col];
        for (int row = 0; row < 8; row++) {
            if (line & (1 << row)) {
                if (size == 1) {
                    platform_set_pixel(x + col, y + row, true);
                } else {
                    platform_fill_rect(x + col * size, y + row * size, size, size, true);
                }
            }
        }
    }
}

void platform_print_char(char c) {
    if (c == '\n') {
        _cursorX = 0;
        _cursorY += 8 * _textSize;
        return;
    }

    draw_char(_cursorX, _cursorY, c, _textSize);
    _cursorX += 6 * _textSize;
}

void platform_print(const char* str) {
    while (*str) {
        platform_print_char(*str++);
    }
}

void platform_print_int(int value) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%d", value);
    platform_print(buf);
}

// ============================================================================
// Display - Sprites/Tiles
// ============================================================================

void platform_draw_tile(uint8_t tileX, uint8_t tileY, const uint8_t* tileData) {
    int pixelX = tileX * PLATFORM_TILE_SIZE;
    int pixelY = tileY * PLATFORM_TILE_SIZE;

    for (int row = 0; row < PLATFORM_TILE_SIZE; row++) {
        uint8_t rowData = platform_pgm_read_byte(&tileData[row]);
        for (int col = 0; col < PLATFORM_TILE_SIZE; col++) {
            if (rowData & (1 << (7 - col))) {  // MSB-first for tiles
                platform_set_pixel(pixelX + col, pixelY + row, true);
            }
        }
    }
}

void platform_draw_sprite_scaled(int x, int y, const uint8_t* spriteData, int scale) {
    for (int row = 0; row < 8; row++) {
        uint8_t rowData = platform_pgm_read_byte(&spriteData[row]);
        for (int col = 0; col < 8; col++) {
            if (rowData & (1 << (7 - col))) {
                platform_fill_rect(x + col * scale, y + row * scale, scale, scale, true);
            }
        }
    }
}

void platform_draw_xbm(int x, int y, int w, int h, const uint8_t* bitmap, bool color) {
    int byteWidth = (w + 7) / 8;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            uint8_t byteVal = platform_pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if (byteVal & (1 << (i % 8))) {  // LSB-first for XBM
                platform_set_pixel(x + i, y + j, color);
            }
        }
    }
}

// ============================================================================
// Input
// ============================================================================

void platform_input_update(void) {
    poll_terminal_input();

    unsigned long now = platform_millis();

    for (int i = 0; i < BTN_COUNT; i++) {
        _buttonCurrent[i] = _buttonEverPressed[i] &&
                            (now - _buttonLastEvent[i]) < KEY_HOLD_MS;

        // Compute edge states BEFORE copying
        _buttonJustPressed[i] = _buttonCurrent[i] && !_buttonPrevious[i];
        _buttonJustReleased[i] = !_buttonCurrent[i] && _buttonPrevious[i];

        // Track press start time
        if (_buttonJustPressed[i]) {
            _buttonPressStart[i] = now;
        }

        // Copy current to previous for next frame
        _buttonPrevious[i] = _buttonCurrent[i];
    }
}

bool platform_button_pressed(PlatformButton btn) {
    return _buttonJustPressed[btn];
}

bool platform_button_released(PlatformButton btn) {
    return _buttonJustReleased[btn];
}

bool platform_button_held(PlatformButton btn) {
    return _buttonCurrent[btn];
}

unsigned long platform_button_held_ms(PlatformButton btn) {
    if (_buttonCurrent[btn]) {
        return platform_millis() - _buttonPressStart[btn];
    }
    return 0;
}

// ============================================================================
// Sound (no tone generation in a plain terminal - all no-ops)
// ============================================================================

void platform_beep(int freq, int duration_ms) {
    (void)freq;
    (void)duration_ms;
}

void platform_play_melody(const int* notes, const int* durations, int length) {
    (void)notes;
    (void)durations;
    (void)length;
}

void platform_set_melody(const int* notes, const int* durations, int length) {
    (void)notes;
    (void)durations;
    (void)length;
}

void platform_update_melody(void) {}

void platform_stop_melody(void) {}

// ============================================================================
// LED (no-op on terminal)
// ============================================================================

void platform_led_set(uint8_t r, uint8_t g, uint8_t b) {
    (void)r;
    (void)g;
    (void)b;
}

void platform_led_off(void) {}

// ============================================================================
// Timing
// ============================================================================

unsigned long platform_millis(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (unsigned long)((now.tv_sec - _startTime.tv_sec) * 1000 +
                           (now.tv_nsec - _startTime.tv_nsec) / 1000000);
}

void platform_delay(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// ============================================================================
// System Controls
// ============================================================================

static uint8_t _systemVolume = 80;      // 0-100
static uint8_t _systemBrightness = 80;  // 0-100

uint8_t platform_get_volume(void) {
    return _systemVolume;
}

void platform_set_volume(uint8_t volume) {
    _systemVolume = volume > 100 ? 100 : volume;
}

uint8_t platform_get_brightness(void) {
    return _systemBrightness;
}

void platform_set_brightness(uint8_t brightness) {
    _systemBrightness = brightness > 100 ? 100 : brightness;
}
