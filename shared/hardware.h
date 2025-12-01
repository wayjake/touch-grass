#ifndef HARDWARE_H
#define HARDWARE_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Global display instance
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RGB LED instance
Adafruit_NeoPixel rgbLed(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

// Button indices
enum Button {
    BUTTON_UP = 0,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_A,
    BUTTON_B,
    BUTTON_COUNT
};

// Button pin mapping
static const uint8_t _buttonPins[BUTTON_COUNT] = {
    BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B
};

// Button states for debouncing
static bool _buttonLastState[BUTTON_COUNT] = {false};
static bool _buttonCurrentState[BUTTON_COUNT] = {false};
static unsigned long _buttonPressStart[BUTTON_COUNT] = {0};

// Initialize all hardware
void hardware_init() {
    // Initialize buzzers
    pinMode(BUZZER_PIN_1, OUTPUT);
    pinMode(BUZZER_PIN_2, OUTPUT);

    // Initialize all buttons with internal pull-up
    for (int i = 0; i < BUTTON_COUNT; i++) {
        pinMode(_buttonPins[i], INPUT_PULLUP);
    }

    // Initialize RGB LED
    rgbLed.begin();
    rgbLed.setBrightness(50);
    rgbLed.clear();
    rgbLed.show();

    // Initialize I2C display
    Wire.begin(I2C_SDA, I2C_SCL);
    display.begin(OLED_ADDR, true);
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
}

// Update all button states (call once per frame at start of loop)
void button_update() {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        _buttonLastState[i] = _buttonCurrentState[i];
        _buttonCurrentState[i] = (digitalRead(_buttonPins[i]) == LOW);

        if (_buttonCurrentState[i] && !_buttonLastState[i]) {
            _buttonPressStart[i] = millis();
        }
    }
}

// Returns true on rising edge (button just pressed)
bool button_pressed(Button btn) {
    return _buttonCurrentState[btn] && !_buttonLastState[btn];
}

// Returns true on falling edge (button just released)
bool button_released(Button btn) {
    return !_buttonCurrentState[btn] && _buttonLastState[btn];
}

// Returns true while button is held down
bool button_held(Button btn) {
    return _buttonCurrentState[btn];
}

// Returns how long button has been held (0 if not held)
unsigned long button_held_ms(Button btn) {
    if (_buttonCurrentState[btn]) {
        return millis() - _buttonPressStart[btn];
    }
    return 0;
}

// Convenience functions for directional input
bool dpad_up()    { return button_held(BUTTON_UP); }
bool dpad_down()  { return button_held(BUTTON_DOWN); }
bool dpad_left()  { return button_held(BUTTON_LEFT); }
bool dpad_right() { return button_held(BUTTON_RIGHT); }

bool dpad_up_pressed()    { return button_pressed(BUTTON_UP); }
bool dpad_down_pressed()  { return button_pressed(BUTTON_DOWN); }
bool dpad_left_pressed()  { return button_pressed(BUTTON_LEFT); }
bool dpad_right_pressed() { return button_pressed(BUTTON_RIGHT); }

// RGB LED functions
void led_set(uint8_t r, uint8_t g, uint8_t b) {
    rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
    rgbLed.show();
}

void led_off() {
    rgbLed.clear();
    rgbLed.show();
}

void led_brightness(uint8_t brightness) {
    rgbLed.setBrightness(brightness);
    rgbLed.show();
}

#endif
