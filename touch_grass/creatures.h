#ifndef TG_CREATURES_H
#define TG_CREATURES_H

#include <stdint.h>
#include <stdlib.h>
#include "../shared/platform.h"
#include "sprites.h"

// Maximum creatures per chunk/map
#define MAX_CREATURES 4

// Creature behavior types
enum CreatureBehavior {
    BEHAVIOR_PASSIVE,      // Moves randomly, won't attack
    BEHAVIOR_TIMID,        // Flees from player
    BEHAVIOR_AGGRESSIVE    // Chases and attacks player
};

// Creature types (based on biome)
enum CreatureType {
    CREATURE_NONE = 0,
    CREATURE_RABBIT,       // Temperate, passive
    CREATURE_WOLF,         // Woodsy, aggressive
    CREATURE_SNAKE,        // Arid, aggressive
    CREATURE_BEAR,         // Snow, aggressive
    CREATURE_TYPE_COUNT
};

// Creature data structure
struct Creature {
    CreatureType type;
    uint8_t x;             // Position X (0-15)
    uint8_t y;             // Position Y (0-7)
    bool alive;
    bool caught;           // Caught by lasso, follows player
};

// Array of creatures on current map
static Creature creatures[MAX_CREATURES];
static uint8_t creatureCount = 0;

// Currently caught creature (follows player)
static int8_t caughtCreatureIndex = -1;  // -1 = none caught

// Get behavior for a creature type
static CreatureBehavior getCreatureBehavior(CreatureType type) {
    switch (type) {
        case CREATURE_RABBIT: return BEHAVIOR_TIMID;
        case CREATURE_WOLF:   return BEHAVIOR_AGGRESSIVE;
        case CREATURE_SNAKE:  return BEHAVIOR_AGGRESSIVE;
        case CREATURE_BEAR:   return BEHAVIOR_AGGRESSIVE;
        default:              return BEHAVIOR_PASSIVE;
    }
}

// Get creature name
static const char* getCreatureName(CreatureType type) {
    switch (type) {
        case CREATURE_RABBIT: return "Rabbit";
        case CREATURE_WOLF:   return "Wolf";
        case CREATURE_SNAKE:  return "Snake";
        case CREATURE_BEAR:   return "Bear";
        default:              return "Unknown";
    }
}

// Get attack damage for a creature type
static uint8_t getCreatureAttackDamage(CreatureType type) {
    switch (type) {
        case CREATURE_RABBIT: return 0;   // Passive, doesn't attack
        case CREATURE_WOLF:   return 15;  // Moderate damage
        case CREATURE_SNAKE:  return 20;  // Higher damage
        case CREATURE_BEAR:   return 25;  // Highest damage
        default:              return 10;
    }
}

// Get tile character for creature sprite lookup
static char getCreatureTileChar(CreatureType type) {
    switch (type) {
        case CREATURE_RABBIT: return TG_RABBIT;
        case CREATURE_WOLF:   return TG_WOLF;
        case CREATURE_SNAKE:  return TG_SNAKE;
        case CREATURE_BEAR:   return TG_BEAR;
        default:              return TG_RABBIT;
    }
}

// Initialize creature system (clear all creatures)
static void initCreatures() {
    creatureCount = 0;
    caughtCreatureIndex = -1;
    for (int i = 0; i < MAX_CREATURES; i++) {
        creatures[i].type = CREATURE_NONE;
        creatures[i].alive = false;
        creatures[i].caught = false;
    }
}

// Spawn creatures for the current map based on biome
// For now, spawns based on simple random placement
static void spawnCreatures(uint8_t biomeType, uint16_t distanceFromHome) {
    initCreatures();

    // No creatures on the first level (home area)
    if (distanceFromHome == 0) return;

    // Fewer creatures near home, more farther away
    int numCreatures = 1;
    if (distanceFromHome > 3) numCreatures = 2;
    if (distanceFromHome > 6) numCreatures = 3;
    if (numCreatures > MAX_CREATURES) numCreatures = MAX_CREATURES;

    // Creature type based on biome
    CreatureType creatureType;
    switch (biomeType) {
        case 0:  // BIOME_TEMPERATE
            creatureType = CREATURE_RABBIT;
            break;
        case 1:  // BIOME_WOODSY
            creatureType = CREATURE_WOLF;
            break;
        case 2:  // BIOME_ARID
            creatureType = CREATURE_SNAKE;
            break;
        case 3:  // BIOME_SNOW
            creatureType = CREATURE_BEAR;
            break;
        default:
            creatureType = CREATURE_RABBIT;
    }

    for (int i = 0; i < numCreatures; i++) {
        // Try to find a valid spawn position (not water, not on player)
        int attempts = 20;
        while (attempts > 0) {
            uint8_t x = rand() % 16;
            uint8_t y = rand() % 8;

            // For now, just check it's not on the edges
            if (x >= 2 && x <= 13 && y >= 1 && y <= 6) {
                creatures[i].type = creatureType;
                creatures[i].x = x;
                creatures[i].y = y;
                creatures[i].alive = true;
                creatures[i].caught = false;
                creatureCount++;
                break;
            }
            attempts--;
        }
    }
}

// Check if there's a creature at position
static int8_t getCreatureAt(uint8_t x, uint8_t y) {
    for (int i = 0; i < creatureCount; i++) {
        if (creatures[i].alive && !creatures[i].caught &&
            creatures[i].x == x && creatures[i].y == y) {
            return i;
        }
    }
    return -1;
}

// Check if player is adjacent to any creature
static int8_t getAdjacentCreature(uint8_t playerX, uint8_t playerY) {
    for (int i = 0; i < creatureCount; i++) {
        if (creatures[i].alive && !creatures[i].caught) {
            int dx = abs((int)creatures[i].x - (int)playerX);
            int dy = abs((int)creatures[i].y - (int)playerY);
            if (dx + dy == 1) {
                return i;
            }
        }
    }
    return -1;
}

// Move a single creature based on its AI
static void moveCreature(int index, uint8_t playerX, uint8_t playerY) {
    if (index < 0 || index >= creatureCount) return;
    Creature* c = &creatures[index];
    if (!c->alive || c->caught) return;

    CreatureBehavior behavior = getCreatureBehavior(c->type);
    int8_t dx = 0, dy = 0;

    switch (behavior) {
        case BEHAVIOR_PASSIVE:
            // Random movement (25% chance to move in each direction, 0% stay)
            {
                int r = rand() % 5;
                if (r == 0) dx = -1;
                else if (r == 1) dx = 1;
                else if (r == 2) dy = -1;
                else if (r == 3) dy = 1;
                // r == 4: stay still
            }
            break;

        case BEHAVIOR_TIMID:
            // Move away from player
            if (c->x < playerX) dx = -1;
            else if (c->x > playerX) dx = 1;
            if (c->y < playerY) dy = -1;
            else if (c->y > playerY) dy = 1;
            // Pick one direction randomly if both are valid
            if (dx != 0 && dy != 0) {
                if (rand() % 2 == 0) dx = 0;
                else dy = 0;
            }
            break;

        case BEHAVIOR_AGGRESSIVE:
            // Move toward player
            if (c->x < playerX) dx = 1;
            else if (c->x > playerX) dx = -1;
            if (c->y < playerY) dy = 1;
            else if (c->y > playerY) dy = -1;
            // Pick one direction randomly if both are valid
            if (dx != 0 && dy != 0) {
                if (rand() % 2 == 0) dx = 0;
                else dy = 0;
            }
            break;
    }

    // Attempt movement
    if (dx != 0 || dy != 0) {
        int newX = c->x + dx;
        int newY = c->y + dy;

        // Bounds check
        if (newX >= 0 && newX < 16 && newY >= 0 && newY < 8) {
            // Don't move onto player or other creatures
            if (!(newX == playerX && newY == playerY) &&
                getCreatureAt(newX, newY) < 0) {
                c->x = newX;
                c->y = newY;
            }
        }
    }
}

// Update all creatures (called after player action)
// Returns hunger damage if any creature attacks
static uint8_t updateCreatures(uint8_t playerX, uint8_t playerY) {
    uint8_t totalDamage = 0;

    for (int i = 0; i < creatureCount; i++) {
        if (!creatures[i].alive || creatures[i].caught) continue;

        // Move the creature
        moveCreature(i, playerX, playerY);

        // Check if now adjacent to player - attack if aggressive
        int dx = abs((int)creatures[i].x - (int)playerX);
        int dy = abs((int)creatures[i].y - (int)playerY);

        if (dx + dy == 1) {
            CreatureBehavior behavior = getCreatureBehavior(creatures[i].type);
            if (behavior == BEHAVIOR_AGGRESSIVE) {
                uint8_t damage = getCreatureAttackDamage(creatures[i].type);
                totalDamage += damage;
            }
        }
    }

    return totalDamage;
}

// Catch a creature (with lasso)
static bool catchCreature(int8_t index) {
    if (index < 0 || index >= creatureCount) return false;
    if (!creatures[index].alive || creatures[index].caught) return false;
    if (caughtCreatureIndex >= 0) return false;  // Already have one caught

    creatures[index].caught = true;
    caughtCreatureIndex = index;
    return true;
}

// Release caught creature
static void releaseCreature() {
    if (caughtCreatureIndex >= 0 && caughtCreatureIndex < creatureCount) {
        creatures[caughtCreatureIndex].caught = false;
    }
    caughtCreatureIndex = -1;
}

// Update caught creature position to follow player
static void updateCaughtCreature(uint8_t prevPlayerX, uint8_t prevPlayerY) {
    if (caughtCreatureIndex >= 0 && caughtCreatureIndex < creatureCount) {
        creatures[caughtCreatureIndex].x = prevPlayerX;
        creatures[caughtCreatureIndex].y = prevPlayerY;
    }
}

// Check if player has a caught creature
static bool hasCreatureCaught() {
    return caughtCreatureIndex >= 0;
}

// Get caught creature type
static CreatureType getCaughtCreatureType() {
    if (caughtCreatureIndex >= 0 && caughtCreatureIndex < creatureCount) {
        return creatures[caughtCreatureIndex].type;
    }
    return CREATURE_NONE;
}

#endif
