#ifndef TG_TERRAIN_H
#define TG_TERRAIN_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "sprites.h"

// Random helpers: uniform int in [0, max) / [min, max)
static inline int randBelow(int max) {
    return rand() % max;
}

static inline int randBetween(int min, int max) {
    return min + rand() % (max - min);
}

// Terrain map (128 bytes)
char tg_map[MAP_HEIGHT][MAP_WIDTH];

// Player position
uint8_t tg_playerX;
uint8_t tg_playerY;

// What tile is under the player (so we don't lose it)
char tg_underPlayer;

// Player hunger (starts at 250, decreases by 1 per action/move)
uint16_t playerHunger = 250;

// Global action counter for plant growth
uint8_t globalActionCount = 0;

// Total actions since game start (game time)
uint16_t totalActions = 0;

// Death reason for game over screen
typedef enum {
    DEATH_NONE = 0,
    DEATH_DROWNED,
    DEATH_STARVED
} DeathReason;
DeathReason deathReason = DEATH_NONE;

// Generate a river using random walk from one edge to another
void generateRiver() {
    bool horizontal = randBelow(2) == 0;

    if (horizontal) {
        int y = randBetween(1, MAP_HEIGHT - 1);
        for (int x = 0; x < MAP_WIDTH; x++) {
            tg_map[y][x] = TG_WATER;
            int drift = randBelow(3) - 1;
            y += drift;
            if (y < 1) y = 1;
            if (y >= MAP_HEIGHT - 1) y = MAP_HEIGHT - 2;
        }
    } else {
        int x = randBetween(1, MAP_WIDTH - 1);
        for (int y = 0; y < MAP_HEIGHT; y++) {
            tg_map[y][x] = TG_WATER;
            int drift = randBelow(3) - 1;
            x += drift;
            if (x < 1) x = 1;
            if (x >= MAP_WIDTH - 1) x = MAP_WIDTH - 2;
        }
    }
}

// Generate a cluster of trees around a point
void generateTreeCluster(int centerX, int centerY, int size) {
    for (int dy = -size; dy <= size; dy++) {
        for (int dx = -size; dx <= size; dx++) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT) {
                if (tg_map[y][x] == TG_GRASS && randBelow(100) < 60) {
                    tg_map[y][x] = TG_TREE;
                }
            }
        }
    }
}

// Check if a tile type is solid (blocks movement regardless of position)
bool isSolidTile(char tile) {
    // Buildings are walkable (player can enter them)
    return false;
}

// Count adjacent water tiles at a position
uint8_t countAdjacentWater(int x, int y) {
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    uint8_t count = 0;

    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
            if (tg_map[ny][nx] == TG_WATER) {
                count++;
            }
        }
    }
    return count;
}

// Check if water at position is shallow (< 2 adjacent water tiles = swimmable)
bool isShallowWater(int x, int y) {
    if (tg_map[y][x] != TG_WATER) return false;
    return countAdjacentWater(x, y) < 2;
}

// Check if a tile is walkable (for movement)
bool isWalkable(char tile) {
    // Buildings are walkable (player can enter them)
    return tile != TG_WATER;
}

// Check if a position is valid for the player (includes shallow water)
bool isValidPosition(uint8_t x, uint8_t y) {
    if (x >= MAP_WIDTH || y >= MAP_HEIGHT) return false;
    char tile = tg_map[y][x];

    // Solid tiles always block
    if (isSolidTile(tile)) return false;

    // Water is passable if shallow
    if (tile == TG_WATER) {
        return isShallowWater(x, y);
    }

    // New biome tiles - all walkable
    // Mountains are walkable (extra energy cost handled elsewhere)
    // Snow and ice are walkable
    if (tile == TG_MOUNTAIN || tile == TG_SNOW || tile == TG_ICE) {
        return true;
    }

    return true;
}

// Check if player has any adjacent escapable tile (for drowning check)
bool hasAdjacentLand() {
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; i++) {
        int nx = tg_playerX + dx[i];
        int ny = tg_playerY + dy[i];

        if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) {
            continue;
        }

        char tile = tg_map[ny][nx];

        // Can escape to land tiles
        if (isWalkable(tile)) {
            return true;
        }

        // Can escape to shallow water
        if (tile == TG_WATER && isShallowWater(nx, ny)) {
            return true;
        }
    }
    return false;
}

// Generate the entire terrain
void generateTerrain() {
    // Seed random from analog noise
    
    // Reset player stats
    playerHunger = 250;
    globalActionCount = 0;
    totalActions = 0;
    deathReason = DEATH_NONE;

    // Step 1: Fill everything with grass
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            tg_map[y][x] = TG_GRASS;
        }
    }

    // Step 2: Generate 1-2 rivers
    int numRivers = randBetween(1, 3);
    for (int i = 0; i < numRivers; i++) {
        generateRiver();
    }

    // Step 3: Generate 2-4 tree clusters
    int numClusters = randBetween(2, 5);
    for (int i = 0; i < numClusters; i++) {
        int cx = randBetween(2, MAP_WIDTH - 2);
        int cy = randBetween(1, MAP_HEIGHT - 1);
        generateTreeCluster(cx, cy, randBetween(1, 3));
    }

    // Step 4: Scatter dirt patches (~10% of remaining grass)
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (tg_map[y][x] == TG_GRASS && randBelow(100) < 10) {
                tg_map[y][x] = TG_DIRT;
            }
        }
    }

    // Step 5: Place one chest on a grass tile within visible viewport (columns 3-12)
    bool chestPlaced = false;
    for (int attempts = 0; attempts < 100 && !chestPlaced; attempts++) {
        int cx = randBetween(3, 13);  // Visible viewport columns only
        int cy = randBetween(0, MAP_HEIGHT);
        if (tg_map[cy][cx] == TG_GRASS) {
            tg_map[cy][cx] = TG_CHEST;
            chestPlaced = true;
        }
    }

    // Step 6: Place player in center (or nearby if blocked)
    tg_playerX = MAP_WIDTH / 2;
    tg_playerY = MAP_HEIGHT / 2;

    if (!isValidPosition(tg_playerX, tg_playerY)) {
        // Spiral search outward
        for (int radius = 1; radius < MAP_WIDTH; radius++) {
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int nx = tg_playerX + dx;
                    int ny = tg_playerY + dy;
                    if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT) {
                        if (isValidPosition(nx, ny)) {
                            tg_playerX = nx;
                            tg_playerY = ny;
                            goto found;
                        }
                    }
                }
            }
        }
        found:;
    }

    // Store what's under the player
    tg_underPlayer = tg_map[tg_playerY][tg_playerX];
}

// Move player in direction, returns true if moved
bool movePlayer(int8_t dx, int8_t dy) {
    int newX = tg_playerX + dx;
    int newY = tg_playerY + dy;

    if (newX < 0 || newX >= MAP_WIDTH) return false;
    if (newY < 0 || newY >= MAP_HEIGHT) return false;
    if (!isValidPosition(newX, newY)) return false;

    // Update position
    tg_playerX = newX;
    tg_playerY = newY;

    // Store what's now under the player
    tg_underPlayer = tg_map[tg_playerY][tg_playerX];

    return true;
}

// Get the tile name for display
const char* getTileName(char tile) {
    switch (tile) {
        case TG_GRASS:    return "Grass";
        case TG_WATER:    return "Water";
        case TG_DIRT:     return "Dirt";
        case TG_TREE:     return "Tree";
        case TG_CHEST:    return "Chest";
        case TG_BUILDING: return "Building";
        case TG_SHRUB:    return "Shrub";
        case TG_SEEDLING: return "Seedling";
        case TG_MOUNTAIN: return "Mountain";
        case TG_SNOW:     return "Snow";
        case TG_ICE:      return "Ice";
        default:          return "Unknown";
    }
}

// Modify the tile under the player
void setTileUnderPlayer(char newTile) {
    tg_map[tg_playerY][tg_playerX] = newTile;
    tg_underPlayer = newTile;
}

// Check if a 2x2 area can be built on (all grass or dirt, player at top-left)
bool canBuild2x2() {
    // Player is at top-left, building extends down and right
    // Check bounds
    if (tg_playerX + 1 >= MAP_WIDTH || tg_playerY + 1 >= MAP_HEIGHT) return false;

    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int x = tg_playerX + dx;
            int y = tg_playerY + dy;
            char tile = tg_map[y][x];
            // Allow building on grass or dirt only
            if (tile != TG_GRASS && tile != TG_DIRT) return false;
        }
    }
    return true;
}

// Place a 2x2 building (fills with building tiles, player at top-left)
void placeBuilding2x2() {
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int x = tg_playerX + dx;
            int y = tg_playerY + dy;
            if (x < MAP_WIDTH && y < MAP_HEIGHT) {
                tg_map[y][x] = TG_BUILDING;
            }
        }
    }
    tg_underPlayer = TG_BUILDING;
}

// Decrement hunger, returns true if player starved
bool decrementHunger() {
    if (playerHunger > 0) {
        playerHunger--;
    }
    return playerHunger == 0;
}

// Restore hunger (from eating)
void restoreHunger(uint8_t amount) {
    playerHunger += amount;
    if (playerHunger > 250) {
        playerHunger = 250;
    }
}

// Increment action counter, returns true if growth should trigger
bool incrementActionCounter() {
    globalActionCount++;
    totalActions++;
    if (globalActionCount >= ACTIONS_TO_GROW) {
        globalActionCount = 0;
        return true;
    }
    return false;
}

// Grow all seedlings into shrubs
void growSeedlings() {
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (tg_map[y][x] == TG_SEEDLING) {
                tg_map[y][x] = TG_SHRUB;
            }
        }
    }
    // Update what's under player if they're on a seedling
    if (tg_underPlayer == TG_SEEDLING) {
        tg_underPlayer = TG_SHRUB;
    }
}

#endif
