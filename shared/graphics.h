#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// Requires display to be declared externally
extern Adafruit_SH1106G display;

// Draw XBM bitmap with LSB-to-MSB conversion
// XBM format uses LSB-first bit ordering, but Adafruit's drawBitmap()
// expects MSB-first. This function handles the conversion.
void drawXbm(int16_t x, int16_t y, int16_t width, int16_t height,
             const uint8_t *bitmap, uint16_t color) {
    int16_t byteWidth = (width + 7) / 8;
    for (int16_t j = 0; j < height; j++) {
        for (int16_t i = 0; i < width; i++) {
            uint8_t byteVal = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);
            if (byteVal & (1 << (i % 8))) {
                display.drawPixel(x + i, y + j, color);
            }
        }
    }
}

// Draw an 8x8 tile at tile coordinates (not pixel coordinates)
// tileData should be 8 bytes in PROGMEM, one byte per row
void drawTile(uint8_t tileX, uint8_t tileY, const uint8_t* tileData) {
    int16_t pixelX = tileX * TILE_SIZE;
    int16_t pixelY = tileY * TILE_SIZE;

    for (int row = 0; row < TILE_SIZE; row++) {
        uint8_t rowData = pgm_read_byte(&tileData[row]);
        for (int col = 0; col < TILE_SIZE; col++) {
            if (rowData & (1 << (7 - col))) {  // MSB-first for tiles
                display.drawPixel(pixelX + col, pixelY + row, SH110X_WHITE);
            }
        }
    }
}

#endif
