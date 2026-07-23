#ifndef TG_PROGRESSION_H
#define TG_PROGRESSION_H

#include <stdint.h>

// Progression state tracking for game unlocks
typedef struct {
    bool hasBuiltHouse;         // Player completed a 2x2 building
    bool hasEnteredHouse;       // Player entered their house
    bool hasExitedHouse;        // Player exited (triggers spirit spawn)
    bool spiritSpawned;         // Spirit is on the map
    bool spiritDialogComplete;  // Dialog finished
    bool fovUnlocked;           // Full 16-column view enabled
    bool fovAnimationDone;      // Blink animation completed

    // Spirit location (chunk-relative)
    int8_t spiritX;
    int8_t spiritY;

    // Home location (for distance calculations and biomes)
    int16_t homeChunkX;
    int16_t homeChunkY;
    uint8_t homeLocalX;
    uint8_t homeLocalY;
} ProgressionState;

// Global progression state
static ProgressionState progression;

// Initialize progression to default state
static void initProgression() {
    progression.hasBuiltHouse = false;
    progression.hasEnteredHouse = false;
    progression.hasExitedHouse = false;
    progression.spiritSpawned = false;
    progression.spiritDialogComplete = false;
    progression.fovUnlocked = false;
    progression.fovAnimationDone = false;
    progression.spiritX = -1;
    progression.spiritY = -1;
    progression.homeChunkX = 0;
    progression.homeChunkY = 0;
    progression.homeLocalX = 0;
    progression.homeLocalY = 0;
}

// Pack progression into a single byte for saving
static uint8_t packProgressionFlags() {
    return (progression.hasBuiltHouse ? 0x01 : 0) |
           (progression.hasEnteredHouse ? 0x02 : 0) |
           (progression.hasExitedHouse ? 0x04 : 0) |
           (progression.spiritSpawned ? 0x08 : 0) |
           (progression.spiritDialogComplete ? 0x10 : 0) |
           (progression.fovUnlocked ? 0x20 : 0) |
           (progression.fovAnimationDone ? 0x40 : 0);
}

// Unpack progression from a saved byte
static void unpackProgressionFlags(uint8_t flags) {
    progression.hasBuiltHouse = (flags & 0x01) != 0;
    progression.hasEnteredHouse = (flags & 0x02) != 0;
    progression.hasExitedHouse = (flags & 0x04) != 0;
    progression.spiritSpawned = (flags & 0x08) != 0;
    progression.spiritDialogComplete = (flags & 0x10) != 0;
    progression.fovUnlocked = (flags & 0x20) != 0;
    progression.fovAnimationDone = (flags & 0x40) != 0;
}

#endif
