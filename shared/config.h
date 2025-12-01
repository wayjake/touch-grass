#ifndef CONFIG_H
#define CONFIG_H

// Hardware pins - Buttons
#define BTN_UP          38
#define BTN_DOWN        35
#define BTN_LEFT        36
#define BTN_RIGHT       37
#define BTN_A           19
#define BTN_B           20

// Hardware pins - Audio
#define BUZZER_PIN_1    5
#define BUZZER_PIN_2    40

// Hardware pins - RGB LED (WS2812B)
#define RGB_LED_PIN     1
#define RGB_LED_COUNT   1

// Hardware pins - I2C Display
#define I2C_SDA         41
#define I2C_SCL         42
#define OLED_ADDR       0x3C

// Screen dimensions
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64

// Tile system
#define TILE_SIZE       8
#define MAP_WIDTH       (SCREEN_WIDTH / TILE_SIZE)   // 16 tiles
#define MAP_HEIGHT      (SCREEN_HEIGHT / TILE_SIZE)  // 8 tiles

// Game constants
#define MAX_HUNGER      250
#define HUNGER_RESTORE  25      // Amount restored by eating fruit
#define ACTIONS_TO_GROW 35      // Actions before seedlings become shrubs

#endif
