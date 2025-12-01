#ifndef TG_BUILDING_H
#define TG_BUILDING_H

#include <Arduino.h>
#include "../shared/config.h"
#include "sprites.h"

// Building interior dimensions (6x5 interior + walls = 8x7 display)
#define INTERIOR_WIDTH  6
#define INTERIOR_HEIGHT 5

// Building interior map
char building_map[INTERIOR_HEIGHT][INTERIOR_WIDTH];

// Player position inside building
uint8_t building_playerX;
uint8_t building_playerY;

// What tile is under player in building
char building_underPlayer;

// Stove fire state
bool stove_lit = false;

// Current building's world position (for reference)
uint8_t current_building_x;
uint8_t current_building_y;

// Initialize building interior layout
void initBuildingInterior() {
    // Fill with floor
    for (int y = 0; y < INTERIOR_HEIGHT; y++) {
        for (int x = 0; x < INTERIOR_WIDTH; x++) {
            building_map[y][x] = TG_FLOOR;
        }
    }

    // Place furniture
    // Bed in top-left (takes 2 tiles horizontally)
    building_map[0][0] = TG_BED;
    building_map[0][1] = TG_BED;

    // Desk in top-right (takes 2 tiles horizontally)
    building_map[0][4] = TG_DESK;
    building_map[0][5] = TG_DESK;

    // Stove in bottom-left
    building_map[3][0] = stove_lit ? TG_STOVE_LIT : TG_STOVE;

    // Door in bottom-right
    building_map[4][5] = TG_DOOR;

    // Player starts near door
    building_playerX = 4;
    building_playerY = 4;
    building_underPlayer = building_map[building_playerY][building_playerX];
}

// Check if interior position is walkable
bool isInteriorWalkable(int x, int y) {
    if (x < 0 || x >= INTERIOR_WIDTH || y < 0 || y >= INTERIOR_HEIGHT) {
        return false;
    }
    char tile = building_map[y][x];
    // Can walk on floor and door
    return tile == TG_FLOOR || tile == TG_DOOR;
}

// Move player inside building
bool movePlayerInterior(int8_t dx, int8_t dy) {
    int newX = building_playerX + dx;
    int newY = building_playerY + dy;

    if (!isInteriorWalkable(newX, newY)) {
        return false;
    }

    building_playerX = newX;
    building_playerY = newY;
    building_underPlayer = building_map[building_playerY][building_playerX];

    return true;
}

// Get the tile player is facing (for interactions)
char getFacingTile(int8_t dx, int8_t dy) {
    int x = building_playerX + dx;
    int y = building_playerY + dy;
    if (x < 0 || x >= INTERIOR_WIDTH || y < 0 || y >= INTERIOR_HEIGHT) {
        return TG_WALL;
    }
    return building_map[y][x];
}

// Get name of interior tile
const char* getInteriorTileName(char tile) {
    switch (tile) {
        case TG_FLOOR:     return "Floor";
        case TG_BED:       return "Bed";
        case TG_DESK:      return "Computer";
        case TG_STOVE:     return "Stove";
        case TG_STOVE_LIT: return "Stove (lit)";
        case TG_DOOR:      return "Door";
        case TG_WALL:      return "Wall";
        default:           return "Unknown";
    }
}

// Light the stove
void lightStove() {
    stove_lit = true;
    // Update stove tile in map
    for (int y = 0; y < INTERIOR_HEIGHT; y++) {
        for (int x = 0; x < INTERIOR_WIDTH; x++) {
            if (building_map[y][x] == TG_STOVE) {
                building_map[y][x] = TG_STOVE_LIT;
            }
        }
    }
}

// Check if player is on door
bool isOnDoor() {
    return building_underPlayer == TG_DOOR;
}

// Check if player is adjacent to a specific tile type
bool isAdjacentTo(char tileType) {
    // Check all 4 directions
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    for (int i = 0; i < 4; i++) {
        int x = building_playerX + dx[i];
        int y = building_playerY + dy[i];
        if (x >= 0 && x < INTERIOR_WIDTH && y >= 0 && y < INTERIOR_HEIGHT) {
            if (building_map[y][x] == tileType) {
                return true;
            }
        }
    }
    return false;
}

#endif
