/*
 * Arduino.h shim for Emscripten/WASM build
 *
 * Provides the minimal Arduino-like types and functions needed
 * by the game modules (terrain.h, sprites.h, etc.)
 */

#ifndef ARDUINO_H_SHIM
#define ARDUINO_H_SHIM

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// PROGMEM is a no-op on non-AVR platforms
#define PROGMEM

// pgm_read functions are just direct memory access
#define pgm_read_byte(addr) (*(const uint8_t*)(addr))
#define pgm_read_word(addr) (*(const uint16_t*)(addr))

// Arduino random() - returns 0 to max-1
static inline long random(long max) {
    return rand() % max;
}

// Arduino random(min, max) - returns min to max-1
static inline long random2(long min, long max) {
    return min + (rand() % (max - min));
}

// Overload trick for C - use macro to select version
#define GET_RANDOM_MACRO(_1, _2, NAME, ...) NAME
#define random(...) GET_RANDOM_MACRO(__VA_ARGS__, random2, random)(__VA_ARGS__)

// randomSeed - seed the random number generator
static inline void randomSeed(unsigned long seed) {
    srand((unsigned int)seed);
}

// analogRead - fake analog read (just return noise for seeding)
static inline int analogRead(int pin) {
    (void)pin;
    return rand();
}

// micros - fake microseconds timer
static inline unsigned long micros(void) {
    return (unsigned long)rand();
}

// millis - fake milliseconds timer
static inline unsigned long millis(void) {
    return (unsigned long)(rand() / 1000);
}

// min/max macros
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif // ARDUINO_H_SHIM
