/*
 * Platform Implementation for ESP32 + Adafruit GFX
 *
 * This wraps the existing hardware.h, graphics.h, and sound.h functionality
 * behind the platform abstraction API.
 */

#ifndef PLATFORM_ESP32_H
#define PLATFORM_ESP32_H

#include "platform.h"

#ifdef ARDUINO

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>

// ============================================================================
// Hardware Pin Configuration (from config.h)
// ============================================================================

// Buttons
#define PIN_BTN_UP          38
#define PIN_BTN_DOWN        35
#define PIN_BTN_LEFT        36
#define PIN_BTN_RIGHT       37
#define PIN_BTN_A           19
#define PIN_BTN_B           20

// Audio
#define PIN_BUZZER_1        5
#define PIN_BUZZER_2        40

// RGB LED
#define PIN_RGB_LED         1
#define RGB_LED_COUNT       1

// I2C Display
#define PIN_I2C_SDA         41
#define PIN_I2C_SCL         42
#define OLED_I2C_ADDR       0x3C

// ============================================================================
// Hardware Instances
// ============================================================================

static Adafruit_SH1106G _display(PLATFORM_SCREEN_WIDTH, PLATFORM_SCREEN_HEIGHT, &Wire, -1);
static Adafruit_NeoPixel _rgbLed(RGB_LED_COUNT, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

// ============================================================================
// Button State
// ============================================================================

static const uint8_t _buttonPins[BTN_COUNT] = {
    PIN_BTN_UP, PIN_BTN_DOWN, PIN_BTN_LEFT, PIN_BTN_RIGHT, PIN_BTN_A, PIN_BTN_B
};

static bool _buttonLastState[BTN_COUNT] = {false};
static bool _buttonCurrentState[BTN_COUNT] = {false};
static unsigned long _buttonPressStart[BTN_COUNT] = {0};

// ============================================================================
// Sound State (for non-blocking melody)
// ============================================================================

static int _bgMelodyIndex = 0;
static unsigned long _bgNoteStartTime = 0;
static int _bgCurrentNoteDuration = 0;
static const int* _bgNotes = nullptr;
static const int* _bgDurations = nullptr;
static int _bgMelodyLen = 0;

// ============================================================================
// Initialization
// ============================================================================

void platform_init(void) {
    // Initialize buzzers
    pinMode(PIN_BUZZER_1, OUTPUT);
    pinMode(PIN_BUZZER_2, OUTPUT);

    // Initialize buttons with internal pull-up
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(_buttonPins[i], INPUT_PULLUP);
    }

    // Initialize RGB LED
    _rgbLed.begin();
    _rgbLed.setBrightness(50);
    _rgbLed.clear();
    _rgbLed.show();

    // Initialize I2C display
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    _display.begin(OLED_I2C_ADDR, true);
    _display.clearDisplay();
    _display.setTextColor(SH110X_WHITE);
}

// ============================================================================
// Display - Core
// ============================================================================

void platform_clear_screen(void) {
    _display.clearDisplay();
}

void platform_render(void) {
    _display.display();
}

// ============================================================================
// Display - Primitives
// ============================================================================

void platform_set_pixel(int x, int y, bool color) {
    _display.drawPixel(x, y, color ? SH110X_WHITE : SH110X_BLACK);
}

void platform_draw_hline(int x, int y, int w, bool color) {
    _display.drawFastHLine(x, y, w, color ? SH110X_WHITE : SH110X_BLACK);
}

void platform_draw_vline(int x, int y, int h, bool color) {
    _display.drawFastVLine(x, y, h, color ? SH110X_WHITE : SH110X_BLACK);
}

void platform_draw_line(int x0, int y0, int x1, int y1, bool color) {
    _display.drawLine(x0, y0, x1, y1, color ? SH110X_WHITE : SH110X_BLACK);
}

void platform_draw_rect(int x, int y, int w, int h, bool color) {
    _display.drawRect(x, y, w, h, color ? SH110X_WHITE : SH110X_BLACK);
}

void platform_fill_rect(int x, int y, int w, int h, bool color) {
    _display.fillRect(x, y, w, h, color ? SH110X_WHITE : SH110X_BLACK);
}

// ============================================================================
// Display - Text
// ============================================================================

void platform_set_cursor(int x, int y) {
    _display.setCursor(x, y);
}

void platform_set_text_size(uint8_t size) {
    _display.setTextSize(size);
}

void platform_print(const char* str) {
    _display.print(str);
}

void platform_print_char(char c) {
    _display.print(c);
}

void platform_print_int(int value) {
    _display.print(value);
}

// ============================================================================
// Display - Sprites/Tiles
// ============================================================================

void platform_draw_tile(uint8_t tileX, uint8_t tileY, const uint8_t* tileData) {
    int16_t pixelX = tileX * PLATFORM_TILE_SIZE;
    int16_t pixelY = tileY * PLATFORM_TILE_SIZE;

    for (int row = 0; row < PLATFORM_TILE_SIZE; row++) {
        uint8_t rowData = pgm_read_byte(&tileData[row]);
        for (int col = 0; col < PLATFORM_TILE_SIZE; col++) {
            if (rowData & (1 << (7 - col))) {  // MSB-first for tiles
                _display.drawPixel(pixelX + col, pixelY + row, SH110X_WHITE);
            }
        }
    }
}

void platform_draw_sprite_scaled(int x, int y, const uint8_t* spriteData, int scale) {
    for (int row = 0; row < 8; row++) {
        uint8_t rowData = pgm_read_byte(&spriteData[row]);
        for (int col = 0; col < 8; col++) {
            if (rowData & (1 << (7 - col))) {
                _display.fillRect(x + col * scale, y + row * scale, scale, scale, SH110X_WHITE);
            }
        }
    }
}

void platform_draw_xbm(int x, int y, int w, int h, const uint8_t* bitmap, bool color) {
    int16_t byteWidth = (w + 7) / 8;
    uint16_t gfxColor = color ? SH110X_WHITE : SH110X_BLACK;

    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            uint8_t byteVal = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if (byteVal & (1 << (i % 8))) {  // LSB-first for XBM
                _display.drawPixel(x + i, y + j, gfxColor);
            }
        }
    }
}

// ============================================================================
// Input
// ============================================================================

void platform_input_update(void) {
    for (int i = 0; i < BTN_COUNT; i++) {
        _buttonLastState[i] = _buttonCurrentState[i];
        _buttonCurrentState[i] = (digitalRead(_buttonPins[i]) == LOW);

        if (_buttonCurrentState[i] && !_buttonLastState[i]) {
            _buttonPressStart[i] = millis();
        }
    }
}

bool platform_button_pressed(PlatformButton btn) {
    return _buttonCurrentState[btn] && !_buttonLastState[btn];
}

bool platform_button_released(PlatformButton btn) {
    return !_buttonCurrentState[btn] && _buttonLastState[btn];
}

bool platform_button_held(PlatformButton btn) {
    return _buttonCurrentState[btn];
}

unsigned long platform_button_held_ms(PlatformButton btn) {
    if (_buttonCurrentState[btn]) {
        return millis() - _buttonPressStart[btn];
    }
    return 0;
}

// ============================================================================
// System Controls (declared early so sound functions can use volume)
// ============================================================================

static uint8_t _systemVolume = 80;      // 0-100
static uint8_t _systemBrightness = 80;  // 0-100

// ============================================================================
// Sound (using LEDC for volume control via duty cycle)
// ============================================================================

#define BUZZER_LEDC_RESOLUTION 8  // 8-bit = 0-255 duty cycle

static bool _buzzerInitialized = false;

static void initBuzzer() {
    if (!_buzzerInitialized) {
        // ESP32 Arduino Core 3.x API: ledcAttach(pin, freq, resolution)
        ledcAttach(PIN_BUZZER_1, 1000, BUZZER_LEDC_RESOLUTION);
        _buzzerInitialized = true;
    }
}

void platform_beep(int freq, int duration_ms) {
    if (_systemVolume == 0) return;  // Muted
    initBuzzer();

    // Duty cycle based on volume (0-100 -> 0-127, max 50% duty for square wave)
    uint8_t duty = (_systemVolume * 127) / 100;

    ledcWriteTone(PIN_BUZZER_1, freq);
    ledcWrite(PIN_BUZZER_1, duty);
    delay(duration_ms);
    ledcWrite(PIN_BUZZER_1, 0);
}

void platform_play_melody(const int* notes, const int* durations, int length) {
    if (_systemVolume == 0) return;  // Muted
    initBuzzer();

    uint8_t duty = (_systemVolume * 127) / 100;

    for (int i = 0; i < length; i++) {
        int note = pgm_read_word(&notes[i]);
        int duration = pgm_read_word(&durations[i]);

        if (note == 0) {  // NOTE_REST
            ledcWrite(PIN_BUZZER_1, 0);
        } else {
            ledcWriteTone(PIN_BUZZER_1, note);
            ledcWrite(PIN_BUZZER_1, duty);
        }
        delay(duration + 30);
    }
    ledcWrite(PIN_BUZZER_1, 0);
}

void platform_set_melody(const int* notes, const int* durations, int length) {
    initBuzzer();
    _bgNotes = notes;
    _bgDurations = durations;
    _bgMelodyLen = length;
    _bgMelodyIndex = 0;
    _bgNoteStartTime = millis();
    _bgCurrentNoteDuration = 0;
    ledcWrite(PIN_BUZZER_1, 0);
}

void platform_update_melody(void) {
    if (_bgNotes == nullptr || _bgMelodyLen == 0) return;
    if (_systemVolume == 0) return;  // Muted

    unsigned long currentTime = millis();

    if (currentTime - _bgNoteStartTime >= (unsigned long)(_bgCurrentNoteDuration + 20)) {
        if (_bgCurrentNoteDuration > 0) {
            _bgMelodyIndex++;
            if (_bgMelodyIndex >= _bgMelodyLen) {
                _bgMelodyIndex = 0;  // Loop
            }
        }

        int note = pgm_read_word(&_bgNotes[_bgMelodyIndex]);
        _bgCurrentNoteDuration = pgm_read_word(&_bgDurations[_bgMelodyIndex]);
        _bgNoteStartTime = currentTime;

        uint8_t duty = (_systemVolume * 127) / 100;

        if (note == 0) {
            ledcWrite(PIN_BUZZER_1, 0);
        } else {
            ledcWriteTone(PIN_BUZZER_1, note);
            ledcWrite(PIN_BUZZER_1, duty);
        }
    }
}

void platform_stop_melody(void) {
    _bgNotes = nullptr;
    _bgDurations = nullptr;
    _bgMelodyLen = 0;
    initBuzzer();
    ledcWrite(PIN_BUZZER_1, 0);
}

// ============================================================================
// LED
// ============================================================================

void platform_led_set(uint8_t r, uint8_t g, uint8_t b) {
    _rgbLed.setPixelColor(0, _rgbLed.Color(r, g, b));
    _rgbLed.show();
}

void platform_led_off(void) {
    _rgbLed.clear();
    _rgbLed.show();
}

// ============================================================================
// System Controls (implementation)
// ============================================================================

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
    // SH1106 supports contrast control (0-255)
    uint8_t contrast = (brightness * 255) / 100;
    _display.setContrast(contrast);
}

// ============================================================================
// Timing
// ============================================================================

unsigned long platform_millis(void) {
    return millis();
}

void platform_delay(int ms) {
    delay(ms);
}

#endif // ARDUINO

#endif // PLATFORM_ESP32_H
