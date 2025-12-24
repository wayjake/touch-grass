/*
 * TouchGrass Game Logic
 *
 * Shared game state machine used by both ESP32 and Web builds.
 * This file contains all game logic, rendering, and input handling.
 */

#ifndef TG_GAME_H
#define TG_GAME_H

#include "../shared/platform.h"
#include "terrain.h"
#include "inventory.h"
#include "building.h"
#include "sprites.h"
#include "t9_dict.h"
#include "t9_input.h"
#include "save.h"
#include "progression.h"
#include "dialog.h"
#include "creatures.h"
#include "chunks.h"

// Note frequencies
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523
#define NOTE_D5   587
#define NOTE_E5   659
#define NOTE_G5   784
#define NOTE_REST 0

// Game states
enum GameState {
    STATE_SPLASH,            // Title splash screen
    STATE_MENU,              // Main menu
    STATE_WORLD,             // Walking around the map
    STATE_TILE_VIEW,         // Zoomed into a tile
    STATE_INVENTORY,         // Inventory screen
    STATE_ITEM_VIEW,         // Inspecting an item
    STATE_GAME_OVER,         // Death screen
    STATE_BUILDING,          // Inside a building
    STATE_BUILDING_INTERACT, // Interacting with building furniture
    STATE_CHEST_OPENING,     // Chest opening animation
    STATE_CHEST_OBTAINED,    // Showing items obtained from chest
    // Save/Load states
    STATE_COMPUTER_MENU,     // Computer interface menu
    STATE_SAVE_SLOTS,        // Browse save slots for saving
    STATE_LOAD_SLOTS,        // Browse save slots for loading
    STATE_T9_INPUT,          // T9 text entry for naming saves
    STATE_CONFIRM_OVERWRITE, // Confirm overwriting existing save
    STATE_SAVE_SUCCESS,      // Save complete confirmation
    STATE_LOAD_CONFIRM,      // Confirm before loading (loses progress)
    // Progression states
    STATE_DIALOG,            // Dialog overlay active
    STATE_FOV_ANIMATION      // FOV unlock blink animation
};

// Game state variables
static GameState gameState = STATE_SPLASH;
static uint8_t menuSelection = 0;
static uint8_t inventoryScroll = 0;
static ItemType selectedItem = ITEM_NONE;
static uint8_t inventoryTab = 0;  // 0 = Inventory, 1 = Status, 2 = System
static uint8_t sysMenuSelection = 0;  // 0 = Volume, 1 = Brightness
static char interactingFurniture = TG_FLOOR;  // Currently interacting furniture

// Save/Load state variables
static uint8_t selectedSlot = 0;           // Currently selected save slot (0-5)
static uint8_t slotScroll = 0;             // Scroll offset for slot list
static bool isSaving = true;               // true = saving, false = loading
static GameState returnAfterT9 = STATE_BUILDING;  // Where to return after T9 input

// Chest animation state
static uint8_t chestAnimFrame = 0;
static unsigned long chestAnimTime = 0;
static const unsigned long CHEST_ANIM_DELAY = 300;  // ms between frames

// FOV unlock animation state
static uint8_t fovAnimFrame = 0;
static unsigned long fovAnimTime = 0;
static const unsigned long FOV_BLINK_DELAY = 150;   // ms between blinks
static const uint8_t FOV_BLINK_COUNT = 6;           // 3 full on/off cycles

// Viewport restriction constants
#define VIEWPORT_HIDDEN_COLS 3    // Columns hidden on each side initially
#define VIEWPORT_VISIBLE_START 3  // First visible column (0-indexed)
#define VIEWPORT_VISIBLE_END 13   // Last visible column (exclusive)

// Movement timing
static unsigned long lastMoveTime = 0;
static const unsigned long MOVE_DELAY = 150;

// Melodies
static const int startupMelody[] PLATFORM_PROGMEM = { NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5 };
static const int startupDurations[] PLATFORM_PROGMEM = { 100, 100, 100, 300 };
#define STARTUP_LEN 4

static const int deathMelody[] PLATFORM_PROGMEM = { NOTE_E4, NOTE_D4, NOTE_C4, NOTE_REST };
static const int deathDurations[] PLATFORM_PROGMEM = { 200, 200, 400, 100 };
#define DEATH_LEN 4

// ============================================================================
// Helper Functions
// ============================================================================

// Check if a 2x2 area starting at (x,y) is all building tiles
static bool isComplete2x2At(int x, int y) {
    if (x < 0 || y < 0 || x + 1 >= MAP_WIDTH || y + 1 >= MAP_HEIGHT) return false;
    return tg_map[y][x] == TG_BUILDING &&
           tg_map[y][x+1] == TG_BUILDING &&
           tg_map[y+1][x] == TG_BUILDING &&
           tg_map[y+1][x+1] == TG_BUILDING;
}

// Check if position is part of any complete 2x2 building
static bool isPartOfCompleteBuilding(int x, int y) {
    if (tg_map[y][x] != TG_BUILDING) return false;
    return isComplete2x2At(x, y) ||
           isComplete2x2At(x-1, y) ||
           isComplete2x2At(x, y-1) ||
           isComplete2x2At(x-1, y-1);
}

// Check if a position is within the visible viewport
static bool isWithinViewport(int x, int y) {
    // If FOV is unlocked, full map is visible
    if (progression.fovUnlocked) {
        return x >= 0 && x < MAP_WIDTH && y >= 0 && y < MAP_HEIGHT;
    }
    // Otherwise, only columns 3-12 are accessible
    return x >= VIEWPORT_VISIBLE_START && x < VIEWPORT_VISIBLE_END &&
           y >= 0 && y < MAP_HEIGHT;
}

// Move player with viewport bounds checking
static bool movePlayerBounded(int8_t dx, int8_t dy) {
    int newX = tg_playerX + dx;
    int newY = tg_playerY + dy;

    // Check viewport bounds first (before terrain checks)
    if (!isWithinViewport(newX, newY)) {
        return false;
    }

    return movePlayer(dx, dy);
}

// Check if position is the top-left corner of a complete 2x2 building
static bool isBuildingTopLeft(int x, int y) {
    if (tg_map[y][x] != TG_BUILDING) return false;
    if (!isComplete2x2At(x, y)) return false;
    bool topClear = (y == 0) || (tg_map[y-1][x] != TG_BUILDING);
    bool leftClear = (x == 0) || (tg_map[y][x-1] != TG_BUILDING);
    return topClear && leftClear;
}

// Check if player is standing on any tile of the complete 2x2 building at (topLeftX, topLeftY)
static bool isPlayerOnBuilding2x2(int topLeftX, int topLeftY) {
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            if ((int)tg_playerX == topLeftX + dx && (int)tg_playerY == topLeftY + dy) {
                return true;
            }
        }
    }
    return false;
}

// Check if position is part of a building but NOT top-left
static bool isBuildingNonOrigin(int x, int y) {
    if (tg_map[y][x] != TG_BUILDING) return false;
    return !isBuildingTopLeft(x, y);
}

// Find the top-left corner of the building the player is in
static void findBuildingTopLeft(int startX, int startY, int* outX, int* outY) {
    *outX = startX;
    *outY = startY;
    // Check all 4 positions
    if (isComplete2x2At(startX, startY)) { *outX = startX; *outY = startY; return; }
    if (startX > 0 && isComplete2x2At(startX-1, startY)) { *outX = startX-1; *outY = startY; return; }
    if (startY > 0 && isComplete2x2At(startX, startY-1)) { *outX = startX; *outY = startY-1; return; }
    if (startX > 0 && startY > 0 && isComplete2x2At(startX-1, startY-1)) { *outX = startX-1; *outY = startY-1; return; }
}

// Spawn the spirit near the player's home
static void spawnSpirit() {
    // Find the building top-left
    int homeX, homeY;
    findBuildingTopLeft(progression.homeLocalX, progression.homeLocalY, &homeX, &homeY);

    // Spirit spawns at bottom-right corner adjacent to building (outside)
    // Building is 2x2 at (homeX, homeY), so bottom-right of building is (homeX+1, homeY+1)
    // Try to spawn at (homeX+2, homeY+1) first, then (homeX+1, homeY+2)

    int spiritX = homeX + 2;
    int spiritY = homeY + 1;

    // Check bounds and if position is valid
    if (spiritX >= MAP_WIDTH || !isValidPosition(spiritX, spiritY)) {
        spiritX = homeX + 1;
        spiritY = homeY + 2;
    }

    // Check if valid
    if (spiritY < MAP_HEIGHT && isValidPosition(spiritX, spiritY)) {
        progression.spiritSpawned = true;
        progression.spiritX = spiritX;
        progression.spiritY = spiritY;
    }
}

// Check if player is adjacent to the spirit
static bool isPlayerAdjacentToSpirit() {
    if (!progression.spiritSpawned || progression.spiritDialogComplete) return false;
    int dx = abs((int)tg_playerX - progression.spiritX);
    int dy = abs((int)tg_playerY - progression.spiritY);
    // Adjacent means Manhattan distance of 1 (not diagonal)
    return (dx + dy == 1);
}

static bool processAction() {
    if (decrementHunger()) {
        deathReason = DEATH_STARVED;
        return true;
    }
    if (incrementActionCounter()) {
        growSeedlings();
    }
    return false;
}

// Transition to a new chunk when player crosses edge
// direction: -1 = left, 1 = right (only horizontal scrolling for now)
static void transitionChunk(int8_t direction) {
    // Save current chunk's terrain back to cache (in case player modified it)
    Chunk* currentChunk = getOrCreateChunk(playerChunkX, playerChunkY, progression.homeChunkX);
    if (currentChunk) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                currentChunk->terrain[y][x] = tg_map[y][x];
            }
        }
        currentChunk->modified = true;
    }

    // Update chunk coordinates
    playerChunkX += direction;

    // Load new chunk
    Chunk* newChunk = getOrCreateChunk(playerChunkX, playerChunkY, progression.homeChunkX);
    if (newChunk) {
        // Copy new chunk terrain to active map
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                tg_map[y][x] = newChunk->terrain[y][x];
            }
        }
    }

    // Move player to opposite edge
    if (direction < 0) {
        tg_playerX = MAP_WIDTH - 1;  // Entering from right
    } else {
        tg_playerX = 0;  // Entering from left
    }

    // Update what's under the player
    tg_underPlayer = tg_map[tg_playerY][tg_playerX];

    // Calculate distance from home for creature spawning
    int16_t distance = (playerChunkX >= progression.homeChunkX)
        ? (playerChunkX - progression.homeChunkX)
        : (progression.homeChunkX - playerChunkX);

    // Get biome and spawn creatures
    BiomeType biome = getBiomeForChunk(playerChunkX, progression.homeChunkX);
    spawnCreatures((uint8_t)biome, (uint16_t)distance);

    platform_beep(NOTE_G4, 50);
    platform_beep(NOTE_C5, 50);
}

static uint8_t getTileActions(char tile, const char** actionNames) {
    switch (tile) {
        case TG_GRASS:
            actionNames[0] = "Cut";
            if (hasItem(ITEM_HAMMER) && getItemCount(ITEM_WOOD) >= 1) {
                actionNames[1] = "Build";
                actionNames[2] = "Examine";
                return 3;
            }
            actionNames[1] = "Examine";
            return 2;
        case TG_DIRT:
            actionNames[0] = "Dig";
            if (hasItem(ITEM_SEED)) {
                actionNames[1] = "Plant";
                if (hasItem(ITEM_HAMMER) && getItemCount(ITEM_WOOD) >= 1) {
                    actionNames[2] = "Build";
                    actionNames[3] = "Examine";
                    return 4;
                }
                actionNames[2] = "Examine";
                return 3;
            }
            if (hasItem(ITEM_HAMMER) && getItemCount(ITEM_WOOD) >= 1) {
                actionNames[1] = "Build";
                actionNames[2] = "Examine";
                return 3;
            }
            actionNames[1] = "Examine";
            return 2;
        case TG_TREE:
            if (hasItem(ITEM_AXE)) {
                actionNames[0] = "Cut";
                actionNames[1] = "Examine";
                return 2;
            }
            actionNames[0] = "Examine";
            return 1;
        case TG_CHEST:
            actionNames[0] = "Open";
            actionNames[1] = "Examine";
            return 2;
        case TG_SHRUB:
            actionNames[0] = "Harvest";
            actionNames[1] = "Examine";
            return 2;
        case TG_SEEDLING:
            actionNames[0] = "Examine";
            return 1;
        case TG_BUILDING:
            if (isPartOfCompleteBuilding(tg_playerX, tg_playerY)) {
                actionNames[0] = "Enter";
                actionNames[1] = "Examine";
                return 2;
            }
            actionNames[0] = "Examine";
            return 1;
        case TG_WATER:
            if (hasItem(ITEM_DIRT)) {
                actionNames[0] = "Place Dirt";
                actionNames[1] = "Examine";
                return 2;
            }
            actionNames[0] = "Examine";
            return 1;
        default:
            actionNames[0] = "Examine";
            return 1;
    }
}

static bool executeTileAction(char tile, uint8_t action) {
    const char* actions[4];
    getTileActions(tile, actions);
    const char* actionName = actions[action];

    if (tile == TG_GRASS && strcmp(actionName, "Cut") == 0) {
        setTileUnderPlayer(TG_DIRT);
        platform_beep(NOTE_E5, 50);
        processAction();
        gameState = STATE_WORLD;
        return true;
    }

    if (tile == TG_TREE && strcmp(actionName, "Cut") == 0) {
        setTileUnderPlayer(TG_DIRT);
        addItem(ITEM_WOOD);
        platform_beep(NOTE_G4, 100);
        platform_beep(NOTE_E4, 100);
        processAction();
        gameState = STATE_WORLD;
        return true;
    }

    if (tile == TG_DIRT && strcmp(actionName, "Dig") == 0) {
        setTileUnderPlayer(TG_WATER);
        addItem(ITEM_DIRT);
        platform_beep(NOTE_C4, 100);
        processAction();
        // Check for drowning
        if (!hasAdjacentLand()) {
            deathReason = DEATH_DROWNED;
            return true;
        }
        gameState = STATE_WORLD;
        return true;
    }

    if (tile == TG_WATER && strcmp(actionName, "Place Dirt") == 0) {
        if (removeItem(ITEM_DIRT)) {
            setTileUnderPlayer(TG_DIRT);
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_C5, 50);
            processAction();
            gameState = STATE_WORLD;
            return true;
        }
        return false;
    }

    if (tile == TG_DIRT && strcmp(actionName, "Plant") == 0) {
        if (removeItem(ITEM_SEED)) {
            setTileUnderPlayer(TG_SEEDLING);
            platform_beep(NOTE_C5, 50);
            platform_beep(NOTE_E5, 50);
            processAction();
            gameState = STATE_WORLD;
            return true;
        }
        return false;
    }

    // Build on grass or dirt - places single building tile
    if ((tile == TG_GRASS || tile == TG_DIRT) && strcmp(actionName, "Build") == 0) {
        if (hasItem(ITEM_HAMMER) && removeItem(ITEM_WOOD, 1)) {
            setTileUnderPlayer(TG_BUILDING);
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_C5, 50);

            // Check if this completed a 2x2 building (first time only)
            if (!progression.hasBuiltHouse && isPartOfCompleteBuilding(tg_playerX, tg_playerY)) {
                progression.hasBuiltHouse = true;
                // Record home location
                progression.homeChunkX = 0;
                progression.homeChunkY = 0;
                progression.homeLocalX = tg_playerX;
                progression.homeLocalY = tg_playerY;
                platform_beep(NOTE_G5, 100);
            }

            processAction();
            gameState = STATE_WORLD;
            return true;
        }
        return false;
    }

    // Open chest - start animation
    if (tile == TG_CHEST && strcmp(actionName, "Open") == 0) {
        chestAnimFrame = 0;
        chestAnimTime = platform_millis();
        gameState = STATE_CHEST_OPENING;
        platform_beep(NOTE_C5, 100);
        return true;
    }

    if (tile == TG_SHRUB && strcmp(actionName, "Harvest") == 0) {
        addItem(ITEM_FRUIT);
        setTileUnderPlayer(TG_SEEDLING);
        platform_beep(NOTE_A4, 50);
        platform_beep(NOTE_C5, 50);
        processAction();
        gameState = STATE_WORLD;
        return true;
    }

    // Enter building - only if complete 2x2
    if (tile == TG_BUILDING && strcmp(actionName, "Enter") == 0) {
        if (!isPartOfCompleteBuilding(tg_playerX, tg_playerY)) {
            return false;
        }
        current_building_x = tg_playerX;
        current_building_y = tg_playerY;
        initBuildingInterior();
        gameState = STATE_BUILDING;
        platform_beep(NOTE_C5, 50);
        platform_beep(NOTE_E5, 50);

        // Track first time entering the house
        if (progression.hasBuiltHouse && !progression.hasEnteredHouse) {
            progression.hasEnteredHouse = true;
        }
        return true;
    }

    // Examine - stays in tile view
    if (strcmp(actionName, "Examine") == 0) {
        platform_beep(NOTE_A4, 80);
        processAction();
        return false;
    }

    return false;
}

static uint8_t getItemActions(ItemType item, const char** actionNames) {
    switch (item) {
        case ITEM_FRUIT:
        case ITEM_COOKED_FRUIT:
            actionNames[0] = "Eat";
            actionNames[1] = "Drop";
            return 2;
        case ITEM_SEED:
            if (tg_underPlayer == TG_DIRT) {
                actionNames[0] = "Plant";
                actionNames[1] = "Drop";
                return 2;
            }
            actionNames[0] = "Drop";
            return 1;
        case ITEM_DIRT:
            if (tg_underPlayer == TG_WATER) {
                actionNames[0] = "Place";
                actionNames[1] = "Drop";
                return 2;
            }
            actionNames[0] = "Drop";
            return 1;
        case ITEM_WOOD:
        case ITEM_HAMMER:
        case ITEM_AXE:
            actionNames[0] = "Drop";
            return 1;
        case ITEM_LASSO:
            // Check if adjacent to a creature
            if (hasCreatureCaught()) {
                actionNames[0] = "Release";
                actionNames[1] = "Drop";
                return 2;
            } else if (getAdjacentCreature(tg_playerX, tg_playerY) >= 0) {
                actionNames[0] = "Catch";
                actionNames[1] = "Drop";
                return 2;
            }
            actionNames[0] = "Drop";
            return 1;
        default:
            return 0;
    }
}

static bool executeItemAction(ItemType item, uint8_t action) {
    const char* actions[4];
    getItemActions(item, actions);
    const char* actionName = actions[action];

    if (item == ITEM_FRUIT && strcmp(actionName, "Eat") == 0) {
        if (removeItem(ITEM_FRUIT)) {
            restoreHunger(HUNGER_RESTORE);
            platform_beep(NOTE_C5, 50);
            platform_beep(NOTE_E5, 50);
            processAction();
        }
        return true;
    }

    if (item == ITEM_COOKED_FRUIT && strcmp(actionName, "Eat") == 0) {
        if (removeItem(ITEM_COOKED_FRUIT)) {
            restoreHunger(HUNGER_RESTORE * 2);
            platform_beep(NOTE_E5, 50);
            platform_beep(NOTE_G5, 50);
            processAction();
        }
        return true;
    }

    if (item == ITEM_SEED && strcmp(actionName, "Plant") == 0) {
        if (tg_underPlayer == TG_DIRT && removeItem(ITEM_SEED)) {
            setTileUnderPlayer(TG_SEEDLING);
            platform_beep(NOTE_C5, 50);
            platform_beep(NOTE_E5, 50);
            processAction();
        }
        return true;
    }

    if (item == ITEM_DIRT && strcmp(actionName, "Place") == 0) {
        if (tg_underPlayer == TG_WATER && removeItem(ITEM_DIRT)) {
            setTileUnderPlayer(TG_DIRT);
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_C5, 50);
            processAction();
        }
        return true;
    }

    // Lasso actions
    if (item == ITEM_LASSO && strcmp(actionName, "Catch") == 0) {
        int8_t creatureIdx = getAdjacentCreature(tg_playerX, tg_playerY);
        if (creatureIdx >= 0 && catchCreature(creatureIdx)) {
            platform_beep(NOTE_G5, 100);
            platform_beep(NOTE_C5, 100);
            processAction();
        }
        return true;
    }

    if (item == ITEM_LASSO && strcmp(actionName, "Release") == 0) {
        releaseCreature();
        platform_beep(NOTE_E4, 100);
        processAction();
        return true;
    }

    if (strcmp(actionName, "Drop") == 0) {
        removeItem(item);
        platform_beep(NOTE_E4, 50);
        return true;
    }

    return false;
}

static uint8_t getFurnitureActions(char furniture, const char** actionNames) {
    switch (furniture) {
        case TG_BED:
            actionNames[0] = "Rest";
            return 1;
        case TG_DESK:
            actionNames[0] = "Save Game";
            return 1;
        case TG_STOVE:
            actionNames[0] = "Light Fire";
            return 1;
        case TG_STOVE_LIT:
            if (hasItem(ITEM_FRUIT)) {
                actionNames[0] = "Cook Fruit";
                actionNames[1] = "Extinguish";
                return 2;
            }
            actionNames[0] = "Extinguish";
            return 1;
        case TG_DOOR:
            actionNames[0] = "Exit";
            return 1;
        default:
            return 0;
    }
}

static bool executeFurnitureAction(char furniture, uint8_t action) {
    const char* actions[4];
    uint8_t numActions = getFurnitureActions(furniture, actions);
    if (action >= numActions) return false;
    const char* actionName = actions[action];

    if (furniture == TG_BED && strcmp(actionName, "Rest") == 0) {
        restoreHunger(10);
        platform_beep(NOTE_C4, 200);
        processAction();
        return true;
    }

    if (furniture == TG_DESK && strcmp(actionName, "Save Game") == 0) {
        platform_beep(NOTE_E5, 50);
        isSaving = true;
        selectedSlot = 0;
        slotScroll = 0;
        loadAllSaveHeaders();
        gameState = STATE_SAVE_SLOTS;
        return false;  // Don't return to building, stay in save flow
    }

    if (furniture == TG_STOVE && strcmp(actionName, "Light Fire") == 0) {
        lightStove();
        platform_beep(NOTE_C5, 100);
        platform_beep(NOTE_E5, 100);
        processAction();
        return true;
    }

    if (furniture == TG_STOVE_LIT && strcmp(actionName, "Cook Fruit") == 0) {
        if (removeItem(ITEM_FRUIT)) {
            addItem(ITEM_COOKED_FRUIT);
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_A4, 50);
            platform_beep(NOTE_C5, 100);
            processAction();
        }
        return true;
    }

    if (furniture == TG_STOVE_LIT && strcmp(actionName, "Extinguish") == 0) {
        stove_lit = false;
        for (int y = 0; y < INTERIOR_HEIGHT; y++) {
            for (int x = 0; x < INTERIOR_WIDTH; x++) {
                if (building_map[y][x] == TG_STOVE_LIT) {
                    building_map[y][x] = TG_STOVE;
                }
            }
        }
        platform_beep(NOTE_E4, 100);
        processAction();
        return true;
    }

    if (furniture == TG_DOOR && strcmp(actionName, "Exit") == 0) {
        gameState = STATE_WORLD;
        platform_beep(NOTE_C5, 50);

        // Spawn spirit on first exit after entering the house
        if (progression.hasEnteredHouse && !progression.hasExitedHouse) {
            progression.hasExitedHouse = true;
            spawnSpirit();
        }
        return true;
    }

    return false;
}

// ============================================================================
// Drawing Functions
// ============================================================================

static void drawScaledSprite(int x, int y, const uint8_t* sprite, int scale) {
    platform_draw_sprite_scaled(x, y, sprite, scale);
}

static void drawTile16(int x, int y, const uint8_t* sprite) {
    // Draw a 16x16 sprite (2 bytes per row, 16 rows)
    for (int row = 0; row < 16; row++) {
        uint8_t byte1 = platform_pgm_read_byte(&sprite[row * 2]);
        uint8_t byte2 = platform_pgm_read_byte(&sprite[row * 2 + 1]);
        for (int col = 0; col < 8; col++) {
            if (byte1 & (1 << (7 - col))) {
                platform_set_pixel(x + col, y + row, true);
            }
            if (byte2 & (1 << (7 - col))) {
                platform_set_pixel(x + col + 8, y + row, true);
            }
        }
    }
}

static void drawSplashScreen() {
    platform_set_text_size(2);
    platform_set_cursor(34, 4);
    platform_print("TOUCH");
    platform_set_cursor(34, 22);
    platform_print("GRASS");

    drawScaledSprite(88, 16, TILE_CHAR, 3);

    platform_set_text_size(1);
    platform_set_cursor(14, 54);
    platform_print("Press A to continue");
}

static uint8_t getMainMenuOptionCount() {
    uint8_t count = 1;  // NEW GAME always present
    if (getMostRecentSave() >= 0) count++;  // CONTINUE
    if (anySaveExists()) count++;  // LOAD
    return count;
}

static const char* getMainMenuOption(uint8_t idx) {
    if (idx == 0) return "NEW GAME";
    uint8_t nextIdx = 1;
    if (getMostRecentSave() >= 0) {
        if (idx == nextIdx) return "CONTINUE";
        nextIdx++;
    }
    if (anySaveExists()) {
        if (idx == nextIdx) return "LOAD";
    }
    return "";
}

static void drawMainMenu() {
    platform_set_text_size(2);
    platform_set_cursor(28, 4);
    platform_print("- MENU -");

    platform_set_text_size(1);
    platform_draw_line(0, 22, 127, 22, true);

    uint8_t optionCount = getMainMenuOptionCount();
    int menuY = 28;

    for (uint8_t i = 0; i < optionCount && i < 3; i++) {
        platform_set_cursor(20, menuY + i * 10);
        if (menuSelection == i) {
            platform_print("> ");
        } else {
            platform_print("  ");
        }
        platform_print(getMainMenuOption(i));
    }

    platform_set_cursor(0, 56);
    platform_print("A:Select");
}

static void drawWorldMap() {
    platform_set_text_size(1);

    // Determine visible column range based on FOV unlock status
    int startCol = 0;
    int endCol = MAP_WIDTH;

    if (!progression.fovUnlocked) {
        startCol = VIEWPORT_VISIBLE_START;  // 3
        endCol = VIEWPORT_VISIBLE_END;      // 13
    }

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            // Skip hidden columns (renders as black since screen is cleared)
            if (x < startCol || x >= endCol) {
                continue;
            }

            char tile = tg_map[y][x];
            bool isPlayerHere = (x == tg_playerX && y == tg_playerY);

            // Handle complete 2x2 buildings specially
            if (tile == TG_BUILDING && isPartOfCompleteBuilding(x, y)) {
                // Find the top-left of this building
                int topLeftX = x, topLeftY = y;
                if (isComplete2x2At(x, y)) {
                    topLeftX = x; topLeftY = y;
                } else if (x > 0 && isComplete2x2At(x-1, y)) {
                    topLeftX = x - 1; topLeftY = y;
                } else if (y > 0 && isComplete2x2At(x, y-1)) {
                    topLeftX = x; topLeftY = y - 1;
                } else if (x > 0 && y > 0 && isComplete2x2At(x-1, y-1)) {
                    topLeftX = x - 1; topLeftY = y - 1;
                }

                // Check if player is on any tile of this building
                if (isPlayerOnBuilding2x2(topLeftX, topLeftY)) {
                    // Player is on building - draw individual 8x8 tiles
                    if (isPlayerHere) {
                        platform_draw_tile(x, y, TILE_CHAR);
                    } else {
                        platform_draw_tile(x, y, TILE_BUILDING);
                    }
                } else {
                    // Player not on building - draw 16x16 cabin from top-left only
                    if (isBuildingTopLeft(x, y)) {
                        drawTile16(x * PLATFORM_TILE_SIZE, y * PLATFORM_TILE_SIZE, TILE_CABIN_16);
                    }
                    // Non-top-left tiles are covered by the 16x16 sprite
                }
                continue;
            }

            // Draw non-building tile content
            if (tile == TG_CHEST) {
                if (!isPlayerHere) {
                    platform_draw_tile(x, y, TILE_CHEST);
                }
            } else if (tile == TG_BUILDING) {
                // Single building tile (not part of complete 2x2)
                if (!isPlayerHere) {
                    platform_draw_tile(x, y, TILE_BUILDING);
                }
            } else if (!isPlayerHere) {
                // Only draw regular tile letter if player isn't here
                platform_set_cursor(x * PLATFORM_TILE_SIZE, y * PLATFORM_TILE_SIZE);
                platform_print_char(tile);
            }

            // Draw player on their tile
            if (isPlayerHere) {
                platform_draw_tile(x, y, TILE_CHAR);
            }
        }
    }

    // Draw spirit if spawned and visible (and not yet talked to)
    if (progression.spiritSpawned && !progression.spiritDialogComplete) {
        int sx = progression.spiritX;
        int sy = progression.spiritY;
        // Only draw if within visible viewport
        if (sx >= startCol && sx < endCol && sy >= 0 && sy < MAP_HEIGHT) {
            platform_draw_tile(sx, sy, TILE_SPIRIT);
        }
    }

    // Draw creatures
    for (int i = 0; i < creatureCount; i++) {
        if (creatures[i].alive) {
            int cx = creatures[i].x;
            int cy = creatures[i].y;
            // Only draw if within visible viewport
            if (cx >= startCol && cx < endCol && cy >= 0 && cy < MAP_HEIGHT) {
                char tileChar = getCreatureTileChar(creatures[i].type);
                const uint8_t* sprite = getTileSprite(tileChar);
                platform_draw_tile(cx, cy, sprite);
            }
        }
    }
}

static void drawTileView() {
    char tile = tg_underPlayer;
    const char* tileName = getTileName(tile);
    const uint8_t* sprite = getTileSprite(tile);

    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print(tileName);

    platform_draw_line(0, 10, 127, 10, true);

    drawScaledSprite(8, 16, sprite, 3);

    const char* actions[4];
    uint8_t numActions = getTileActions(tile, actions);

    int menuX = 50;
    int menuY = 16;
    const uint8_t maxVisible = 3;

    platform_set_cursor(menuX, menuY);
    platform_print("Actions:");

    uint8_t scrollOffset = 0;
    if (menuSelection >= maxVisible) {
        scrollOffset = menuSelection - maxVisible + 1;
    }

    for (uint8_t i = 0; i < maxVisible && (i + scrollOffset) < numActions; i++) {
        uint8_t itemIdx = i + scrollOffset;
        platform_set_cursor(menuX, menuY + 12 + i * 10);
        if (itemIdx == menuSelection) {
            platform_print("> ");
        } else {
            platform_print("  ");
        }
        platform_print(actions[itemIdx]);
    }

    if (scrollOffset > 0) {
        platform_set_cursor(menuX + 70, menuY + 12);
        platform_print("^");
    }
    if (scrollOffset + maxVisible < numActions) {
        platform_set_cursor(menuX + 70, menuY + 12 + (maxVisible - 1) * 10);
        platform_print("v");
    }

    platform_set_cursor(0, 56);
    platform_print("A:Do B:Back");
}

static void drawChestOpening() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print("Chest");
    platform_draw_line(0, 10, 127, 10, true);

    int spriteX = 56;
    int spriteY = 24;

    const uint8_t* frame;
    switch (chestAnimFrame) {
        case 0:
            frame = TILE_CHEST_CLOSED_16;
            break;
        case 1:
            frame = TILE_CHEST_OPENING_16;
            break;
        case 2:
        default:
            frame = TILE_CHEST_OPEN_16;
            break;
    }

    drawTile16(spriteX, spriteY, frame);

    platform_set_cursor(40, 50);
    platform_print("Opening...");
}

static void drawChestObtained() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print("You obtained:");
    platform_draw_line(0, 10, 127, 10, true);

    drawTile16(8, 18, TILE_CHEST_OPEN_16);

    int textX = 32;
    int textY = 18;

    platform_set_cursor(textX, textY);
    platform_print("* 3 Seeds");
    platform_set_cursor(textX, textY + 10);
    platform_print("* Hammer");
    platform_set_cursor(textX, textY + 20);
    platform_print("* Axe");

    platform_set_cursor(20, 56);
    platform_print("Press A or B");
}

static void drawInventoryScreen() {
    platform_set_text_size(1);

    platform_set_cursor(0, 0);
    platform_print("[Inv]");
    platform_set_cursor(40, 0);
    platform_print(" Stat");
    platform_set_cursor(80, 0);
    platform_print(" Sys");
    platform_draw_line(0, 10, 127, 10, true);

    uint8_t filledSlots = countFilledSlots();

    if (filledSlots == 0) {
        platform_set_cursor(20, 28);
        platform_print("(empty)");
    } else {
        int menuY = 14;
        uint8_t displayCount = min((uint8_t)4, filledSlots);

        for (uint8_t i = 0; i < displayCount; i++) {
            uint8_t idx = i + inventoryScroll;
            if (idx >= filledSlots) break;

            int8_t slot = getNthFilledSlot(idx);
            if (slot < 0) continue;

            platform_set_cursor(0, menuY + i * 10);
            if (idx == menuSelection) {
                platform_print("> ");
            } else {
                platform_print("  ");
            }
            platform_print(getItemName(inventory[slot].type));
            if (inventory[slot].count > 1) {
                platform_print(" x");
                platform_print_int(inventory[slot].count);
            }
        }

        if (inventoryScroll > 0) {
            platform_set_cursor(120, 14);
            platform_print("^");
        }
        if (inventoryScroll + 4 < filledSlots) {
            platform_set_cursor(120, 44);
            platform_print("v");
        }
    }

    platform_set_cursor(0, 56);
    platform_print("</>:Tab A:Use B:Back");
}

static void drawStatusScreen() {
    platform_set_text_size(1);

    platform_set_cursor(0, 0);
    platform_print(" Inv");
    platform_set_cursor(40, 0);
    platform_print("[Stat]");
    platform_set_cursor(80, 0);
    platform_print(" Sys");
    platform_draw_line(0, 10, 127, 10, true);

    platform_set_cursor(0, 16);
    platform_print("Hunger: ");
    platform_print_int(playerHunger);
    platform_print("/250");

    int barWidth = 80;
    int barHeight = 6;
    int barX = 10;
    int barY = 28;
    int fillWidth = (playerHunger * barWidth) / 250;
    platform_draw_rect(barX, barY, barWidth, barHeight, true);
    platform_fill_rect(barX, barY, fillWidth, barHeight, true);

    platform_set_cursor(0, 40);
    platform_print("Actions: ");
    platform_print_int(totalActions);

    platform_set_cursor(0, 56);
    platform_print("</>:Tab B:Back");
}

static void drawSystemScreen() {
    platform_set_text_size(1);

    platform_set_cursor(0, 0);
    platform_print(" Inv");
    platform_set_cursor(40, 0);
    platform_print(" Stat");
    platform_set_cursor(80, 0);
    platform_print("[Sys]");
    platform_draw_line(0, 10, 127, 10, true);

    // Volume control
    uint8_t volume = platform_get_volume();
    platform_set_cursor(0, 18);
    if (sysMenuSelection == 0) {
        platform_print(">");
    } else {
        platform_print(" ");
    }
    platform_print("Volume: ");
    platform_print_int(volume);
    platform_print("%");
    // Draw volume bar
    platform_draw_rect(10, 30, 100, 8, true);
    int volFill = (volume * 96) / 100;
    if (volFill > 0) {
        platform_fill_rect(12, 32, volFill, 4, true);
    }

    // Brightness control
    uint8_t brightness = platform_get_brightness();
    platform_set_cursor(0, 42);
    if (sysMenuSelection == 1) {
        platform_print(">");
    } else {
        platform_print(" ");
    }
    platform_print("Bright: ");
    platform_print_int(brightness);
    platform_print("%");
    // Draw brightness bar
    platform_draw_rect(10, 54, 100, 8, true);
    int briFill = (brightness * 96) / 100;
    if (briFill > 0) {
        platform_fill_rect(12, 56, briFill, 4, true);
    }

    platform_set_cursor(0, 56);
    platform_print("</>:Tab ^v:Sel A:Adj");
}

static void drawItemView() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print(getItemName(selectedItem));
    platform_draw_line(0, 10, 127, 10, true);

    const char* actions[4];
    uint8_t numActions = getItemActions(selectedItem, actions);

    int menuX = 10;
    int menuY = 18;

    for (uint8_t i = 0; i < numActions; i++) {
        platform_set_cursor(menuX, menuY + i * 10);
        if (i == menuSelection) {
            platform_print("> ");
        } else {
            platform_print("  ");
        }
        platform_print(actions[i]);
    }

    platform_set_cursor(0, 56);
    platform_print("A:Do B:Back");
}

static void drawGameOver() {
    platform_set_text_size(1);

    platform_set_cursor(32, 10);
    platform_print("GAME OVER");

    platform_set_cursor(20, 30);
    if (deathReason == DEATH_DROWNED) {
        platform_print("You drowned!");
    } else if (deathReason == DEATH_STARVED) {
        platform_print("You starved!");
    }

    platform_set_cursor(12, 50);
    platform_print("Press A to restart");
}

static void drawBuildingInterior() {
    platform_set_text_size(1);

    platform_set_cursor(30, 0);
    platform_print("~ Home ~");

    int offsetX = (128 - (INTERIOR_WIDTH + 2) * 8) / 2;
    int offsetY = 10;

    for (int x = 0; x < INTERIOR_WIDTH + 2; x++) {
        drawScaledSprite(offsetX + x * 8, offsetY, TILE_WALL, 1);
        drawScaledSprite(offsetX + x * 8, offsetY + (INTERIOR_HEIGHT + 1) * 8, TILE_WALL, 1);
    }
    for (int y = 1; y < INTERIOR_HEIGHT + 1; y++) {
        drawScaledSprite(offsetX, offsetY + y * 8, TILE_WALL, 1);
        drawScaledSprite(offsetX + (INTERIOR_WIDTH + 1) * 8, offsetY + y * 8, TILE_WALL, 1);
    }

    for (int y = 0; y < INTERIOR_HEIGHT; y++) {
        for (int x = 0; x < INTERIOR_WIDTH; x++) {
            int screenX = offsetX + (x + 1) * 8;
            int screenY = offsetY + (y + 1) * 8;

            if (x == building_playerX && y == building_playerY) {
                drawScaledSprite(screenX, screenY, TILE_CHAR, 1);
            } else {
                drawScaledSprite(screenX, screenY, getTileSprite(building_map[y][x]), 1);
            }
        }
    }
}

static void drawFurnitureInteract() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print(getInteriorTileName(interactingFurniture));
    platform_draw_line(0, 10, 127, 10, true);

    drawScaledSprite(8, 16, getTileSprite(interactingFurniture), 3);

    const char* actions[4];
    uint8_t numActions = getFurnitureActions(interactingFurniture, actions);

    int menuX = 50;
    int menuY = 16;

    if (numActions > 0) {
        platform_set_cursor(menuX, menuY);
        platform_print("Actions:");

        for (uint8_t i = 0; i < numActions; i++) {
            platform_set_cursor(menuX, menuY + 12 + i * 10);
            if (i == menuSelection) {
                platform_print("> ");
            } else {
                platform_print("  ");
            }
            platform_print(actions[i]);
        }
    }

    platform_set_cursor(0, 56);
    platform_print("A:Do B:Back");
}

// ============================================================================
// Save/Load UI Functions
// ============================================================================

static void drawSlotBrowser() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    if (isSaving) {
        platform_print("SAVE GAME");
    } else {
        platform_print("LOAD GAME");
    }
    platform_draw_line(0, 10, 127, 10, true);

    if (!saveHeadersLoaded) {
        loadAllSaveHeaders();
    }

    const uint8_t maxVisible = 4;
    if (selectedSlot >= slotScroll + maxVisible) {
        slotScroll = selectedSlot - maxVisible + 1;
    } else if (selectedSlot < slotScroll) {
        slotScroll = selectedSlot;
    }

    int menuY = 14;
    for (uint8_t i = 0; i < maxVisible && (i + slotScroll) < SAVE_SLOT_COUNT; i++) {
        uint8_t slotIdx = i + slotScroll;
        platform_set_cursor(0, menuY + i * 10);

        if (slotIdx == selectedSlot) {
            platform_print(">");
        } else {
            platform_print(" ");
        }

        platform_print_int(slotIdx + 1);
        platform_print(". ");

        if (saveHeaders[slotIdx].valid == 0x01) {
            platform_print(saveHeaders[slotIdx].name);
        } else {
            platform_print("[empty]");
        }
    }

    if (slotScroll > 0) {
        platform_set_cursor(120, 14);
        platform_print("^");
    }
    if (slotScroll + maxVisible < SAVE_SLOT_COUNT) {
        platform_set_cursor(120, 44);
        platform_print("v");
    }

    platform_set_cursor(0, 56);
    platform_print("A:Select B:Back");
}

static void drawT9InputScreen() {
    platform_set_text_size(1);

    platform_set_cursor(0, 0);
    const char* text = t9GetText();
    platform_print(text);

    char pending = t9GetPendingChar();
    if (pending) {
        platform_print_char(pending);
    }
    platform_print("_");

    platform_draw_line(0, 9, 127, 9, true);

    // Draw phone keypad grid
    const int gridX = 0;
    const int gridY = 12;
    const int cellW = 14;
    const int cellH = 12;

    const char* keyLabels[4][3] = {
        {"1", "2", "3"},
        {"4", "5", "6"},
        {"7", "8", "9"},
        {"<", "0", "OK"}
    };

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int x = gridX + col * cellW;
            int y = gridY + row * cellH;

            if (col == t9GridX && row == t9GridY) {
                platform_fill_rect(x, y, cellW - 1, cellH - 1, true);
                // Note: inverted text not directly supported, just draw rect
            } else {
                platform_draw_rect(x, y, cellW - 1, cellH - 1, true);
            }

            platform_set_cursor(x + 4, y + 2);
            platform_print(keyLabels[row][col]);
        }
    }

    // Show letters for selected key
    int8_t selectedKey = t9GetSelectedKey();
    platform_set_cursor(46, 12);
    if (selectedKey >= 0 && selectedKey <= 9) {
        platform_print("[");
        platform_print_int(selectedKey);
        platform_print("] ");
        platform_print(T9_KEYS[selectedKey]);
    } else if (selectedKey == -1) {
        platform_print("DELETE");
    } else if (selectedKey == -2) {
        platform_print("CONFIRM");
    }

    // Show predictions
    if (t9PredictionCount > 0 && !t9InPredictionMode) {
        platform_set_cursor(46, 24);
        platform_print("Suggest:");
        char wordBuf[13];
        for (uint8_t i = 0; i < t9PredictionCount && i < 2; i++) {
            platform_set_cursor(46, 34 + i * 10);
            copyDictWord(t9Predictions[i], wordBuf, 10);
            platform_print(wordBuf);
        }
    } else if (t9InPredictionMode) {
        platform_set_cursor(46, 24);
        platform_print("Pick word:");
        char wordBuf[13];
        for (uint8_t i = 0; i < t9PredictionCount && i < 3; i++) {
            platform_set_cursor(46, 34 + i * 10);
            if (i == t9PredictionIndex) {
                platform_print(">");
            } else {
                platform_print(" ");
            }
            copyDictWord(t9Predictions[i], wordBuf, 9);
            platform_print(wordBuf);
        }
    }

    platform_set_cursor(0, 56);
    platform_print("A:Type B:Space/Cancel");
}

static void drawConfirmOverwrite() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print("Overwrite Save?");
    platform_draw_line(0, 10, 127, 10, true);

    platform_set_cursor(0, 16);
    platform_print("Slot ");
    platform_print_int(selectedSlot + 1);
    platform_print(": ");
    platform_print(saveHeaders[selectedSlot].name);

    platform_set_cursor(0, 30);
    platform_print("will be overwritten.");

    platform_set_cursor(10, 44);
    if (menuSelection == 0) {
        platform_print("> Yes   No");
    } else {
        platform_print("  Yes > No");
    }

    platform_set_cursor(0, 56);
    platform_print("A:Select B:Cancel");
}

static void drawLoadConfirm() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print("Load Game?");
    platform_draw_line(0, 10, 127, 10, true);

    platform_set_cursor(0, 16);
    platform_print("Load: ");
    platform_print(saveHeaders[selectedSlot].name);

    platform_set_cursor(0, 30);
    platform_print("Current progress");
    platform_set_cursor(0, 38);
    platform_print("will be lost!");

    platform_set_cursor(10, 48);
    if (menuSelection == 0) {
        platform_print("> Yes   No");
    } else {
        platform_print("  Yes > No");
    }

    platform_set_cursor(0, 56);
    platform_print("A:Select B:Cancel");
}

static void drawSaveSuccess() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print("Game Saved!");
    platform_draw_line(0, 10, 127, 10, true);

    platform_set_cursor(0, 24);
    platform_print("Saved to slot ");
    platform_print_int(selectedSlot + 1);

    platform_set_cursor(0, 36);
    platform_print("Name: ");
    platform_print(saveHeaders[selectedSlot].name);

    platform_set_cursor(0, 56);
    platform_print("A:Continue");
}

// ============================================================================
// Main Game Functions
// ============================================================================

inline void game_setup() {
    initSaveSystem();
    // Game initialization happens when selecting NEW GAME from menu
}

inline void game_loop() {
    platform_input_update();
    platform_clear_screen();

    unsigned long currentTime = platform_millis();

    // Check for death
    if (deathReason != DEATH_NONE && gameState != STATE_GAME_OVER) {
        gameState = STATE_GAME_OVER;
        platform_play_melody(deathMelody, deathDurations, DEATH_LEN);
        platform_led_set(255, 0, 0);
    }

    if (gameState == STATE_SPLASH) {
        drawSplashScreen();

        if (platform_button_pressed(BTN_A)) {
            gameState = STATE_MENU;
            menuSelection = 0;
            platform_beep(NOTE_E5, 50);
        }

    } else if (gameState == STATE_MENU) {
        drawMainMenu();

        uint8_t optionCount = getMainMenuOptionCount();

        if (platform_dpad_up_pressed() && menuSelection > 0) {
            menuSelection--;
            platform_beep(NOTE_G4, 30);
        }
        if (platform_dpad_down_pressed() && menuSelection < optionCount - 1) {
            menuSelection++;
            platform_beep(NOTE_E4, 30);
        }

        if (platform_button_pressed(BTN_A)) {
            const char* option = getMainMenuOption(menuSelection);
            if (strcmp(option, "NEW GAME") == 0) {
                initInventory();
                initProgression();
                initCreatures();
                generateTerrain();
                // Initialize chunk system and save starting terrain as chunk (0,0)
                initChunkSystem(platform_millis() ^ 12345);
                setInitialChunk(tg_map);
                // Spawn some creatures in the starting area (temperate biome)
                spawnCreatures(0, 0);  // biome=temperate, distance=0
                gameState = STATE_WORLD;
                platform_play_melody(startupMelody, startupDurations, STARTUP_LEN);
            } else if (strcmp(option, "CONTINUE") == 0) {
                int8_t slot = getMostRecentSave();
                if (slot >= 0) {
                    int8_t wasInBuilding = loadGame(slot);
                    if (wasInBuilding >= 0) {
                        gameState = wasInBuilding ? STATE_BUILDING : STATE_WORLD;
                        platform_play_melody(startupMelody, startupDurations, STARTUP_LEN);
                    }
                }
            } else if (strcmp(option, "LOAD") == 0) {
                isSaving = false;
                selectedSlot = 0;
                slotScroll = 0;
                loadAllSaveHeaders();
                gameState = STATE_LOAD_SLOTS;
                platform_beep(NOTE_E5, 50);
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_SPLASH;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_WORLD) {
        drawWorldMap();

        if (currentTime - lastMoveTime >= MOVE_DELAY) {
            bool moved = false;
            bool changedChunk = false;

            // Save previous position for caught creature following
            uint8_t prevX = tg_playerX;
            uint8_t prevY = tg_playerY;

            if (platform_dpad_up()) {
                moved = movePlayerBounded(0, -1);
            } else if (platform_dpad_down()) {
                moved = movePlayerBounded(0, 1);
            } else if (platform_dpad_left()) {
                moved = movePlayerBounded(-1, 0);
                // Check for chunk transition (only when FOV unlocked)
                if (!moved && progression.fovUnlocked && tg_playerX == 0) {
                    transitionChunk(-1);  // Move to chunk on the left
                    moved = true;
                    changedChunk = true;
                }
            } else if (platform_dpad_right()) {
                moved = movePlayerBounded(1, 0);
                // Check for chunk transition (only when FOV unlocked)
                if (!moved && progression.fovUnlocked && tg_playerX == MAP_WIDTH - 1) {
                    transitionChunk(1);  // Move to chunk on the right
                    moved = true;
                    changedChunk = true;
                }
            }

            if (moved) {
                if (!changedChunk) {
                    platform_beep(NOTE_C5, 20);
                }
                lastMoveTime = currentTime;

                // Update caught creature to follow player (release if changed chunk)
                if (hasCreatureCaught()) {
                    if (changedChunk) {
                        releaseCreature();  // Can't bring creatures across chunks
                    } else {
                        updateCaughtCreature(prevX, prevY);
                    }
                }

                // Update all creatures (AI movement and attacks)
                uint8_t creatureDamage = updateCreatures(tg_playerX, tg_playerY);
                if (creatureDamage > 0) {
                    // Apply creature attack damage to hunger
                    if (playerHunger > creatureDamage) {
                        playerHunger -= creatureDamage;
                    } else {
                        playerHunger = 0;
                    }
                    platform_beep(NOTE_C4, 100);
                    platform_beep(NOTE_E4, 100);
                }

                if (decrementHunger()) {
                    deathReason = DEATH_STARVED;
                }
                if (incrementActionCounter()) {
                    growSeedlings();
                }
            }
        }

        if (platform_button_pressed(BTN_A)) {
            // Check if adjacent to spirit - start dialog instead
            if (isPlayerAdjacentToSpirit()) {
                startSpiritDialog();
                gameState = STATE_DIALOG;
                platform_beep(NOTE_G5, 80);
            } else {
                gameState = STATE_TILE_VIEW;
                menuSelection = 0;
                platform_beep(NOTE_E5, 50);
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_INVENTORY;
            menuSelection = 0;
            inventoryScroll = 0;
            inventoryTab = 0;
            platform_beep(NOTE_G4, 50);
        }

    } else if (gameState == STATE_TILE_VIEW) {
        drawTileView();

        const char* actions[4];
        uint8_t numActions = getTileActions(tg_underPlayer, actions);

        if (platform_dpad_up_pressed() && menuSelection > 0) {
            menuSelection--;
            platform_beep(NOTE_G4, 30);
        }
        if (platform_dpad_down_pressed() && menuSelection < numActions - 1) {
            menuSelection++;
            platform_beep(NOTE_E4, 30);
        }

        if (platform_button_pressed(BTN_A)) {
            executeTileAction(tg_underPlayer, menuSelection);
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_WORLD;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_INVENTORY) {
        if (platform_dpad_left_pressed() && inventoryTab > 0) {
            inventoryTab--;
            menuSelection = 0;
            inventoryScroll = 0;
            platform_beep(NOTE_C5, 30);
        }
        if (platform_dpad_right_pressed() && inventoryTab < 2) {
            inventoryTab++;
            menuSelection = 0;
            sysMenuSelection = 0;
            platform_beep(NOTE_C5, 30);
        }

        if (inventoryTab == 0) {
            drawInventoryScreen();

            uint8_t filledSlots = countFilledSlots();

            if (filledSlots > 0) {
                if (platform_dpad_up_pressed() && menuSelection > 0) {
                    menuSelection--;
                    if (menuSelection < inventoryScroll) {
                        inventoryScroll = menuSelection;
                    }
                    platform_beep(NOTE_G4, 30);
                }
                if (platform_dpad_down_pressed() && menuSelection < filledSlots - 1) {
                    menuSelection++;
                    if (menuSelection >= inventoryScroll + 4) {
                        inventoryScroll = menuSelection - 3;
                    }
                    platform_beep(NOTE_E4, 30);
                }

                if (platform_button_pressed(BTN_A)) {
                    int8_t slot = getNthFilledSlot(menuSelection);
                    if (slot >= 0) {
                        selectedItem = inventory[slot].type;
                        gameState = STATE_ITEM_VIEW;
                        menuSelection = 0;
                        platform_beep(NOTE_E5, 50);
                    }
                }
            }
        } else if (inventoryTab == 1) {
            drawStatusScreen();
        } else if (inventoryTab == 2) {
            drawSystemScreen();

            // Up/down to select volume or brightness
            if (platform_dpad_up_pressed() && sysMenuSelection > 0) {
                sysMenuSelection--;
                platform_beep(NOTE_G4, 30);
            }
            if (platform_dpad_down_pressed() && sysMenuSelection < 1) {
                sysMenuSelection++;
                platform_beep(NOTE_E4, 30);
            }

            // A button to adjust selected setting (cycles through values)
            if (platform_button_pressed(BTN_A)) {
                if (sysMenuSelection == 0) {
                    // Cycle volume: 0 -> 25 -> 50 -> 75 -> 100 -> 0
                    uint8_t vol = platform_get_volume();
                    vol = (vol >= 100) ? 0 : vol + 25;
                    platform_set_volume(vol);
                    platform_beep(NOTE_E5, 50);
                } else {
                    // Cycle brightness: 20 -> 40 -> 60 -> 80 -> 100 -> 20
                    uint8_t bri = platform_get_brightness();
                    bri = (bri >= 100) ? 20 : bri + 20;
                    platform_set_brightness(bri);
                    platform_beep(NOTE_E5, 50);
                }
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_WORLD;
            inventoryTab = 0;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_ITEM_VIEW) {
        drawItemView();

        const char* actions[4];
        uint8_t numActions = getItemActions(selectedItem, actions);

        if (platform_dpad_up_pressed() && menuSelection > 0) {
            menuSelection--;
            platform_beep(NOTE_G4, 30);
        }
        if (platform_dpad_down_pressed() && menuSelection < numActions - 1) {
            menuSelection++;
            platform_beep(NOTE_E4, 30);
        }

        if (platform_button_pressed(BTN_A)) {
            if (executeItemAction(selectedItem, menuSelection)) {
                gameState = STATE_INVENTORY;
                menuSelection = 0;
                inventoryScroll = 0;
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_INVENTORY;
            menuSelection = 0;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_GAME_OVER) {
        drawGameOver();

        if (platform_button_pressed(BTN_A)) {
            deathReason = DEATH_NONE;
            gameState = STATE_MENU;
            menuSelection = 0;
            platform_led_off();
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_BUILDING) {
        drawBuildingInterior();

        if (currentTime - lastMoveTime >= MOVE_DELAY) {
            bool moved = false;

            if (platform_dpad_up()) {
                moved = movePlayerInterior(0, -1);
            } else if (platform_dpad_down()) {
                moved = movePlayerInterior(0, 1);
            } else if (platform_dpad_left()) {
                moved = movePlayerInterior(-1, 0);
            } else if (platform_dpad_right()) {
                moved = movePlayerInterior(1, 0);
            }

            if (moved) {
                platform_beep(NOTE_C5, 20);
                lastMoveTime = currentTime;
                if (decrementHunger()) {
                    deathReason = DEATH_STARVED;
                }
                if (incrementActionCounter()) {
                    growSeedlings();
                }
            }
        }

        if (platform_button_pressed(BTN_A)) {
            char furniture = TG_FLOOR;

            // Check if standing on interactable furniture
            if (building_underPlayer == TG_DOOR ||
                building_underPlayer == TG_BED ||
                building_underPlayer == TG_DESK ||
                building_underPlayer == TG_STOVE ||
                building_underPlayer == TG_STOVE_LIT) {
                furniture = building_underPlayer;
            } else {
                // Check adjacent tiles
                if (isAdjacentTo(TG_BED)) furniture = TG_BED;
                else if (isAdjacentTo(TG_DESK)) furniture = TG_DESK;
                else if (isAdjacentTo(TG_STOVE_LIT)) furniture = TG_STOVE_LIT;
                else if (isAdjacentTo(TG_STOVE)) furniture = TG_STOVE;
            }

            if (furniture != TG_FLOOR) {
                interactingFurniture = furniture;
                gameState = STATE_BUILDING_INTERACT;
                menuSelection = 0;
                platform_beep(NOTE_E5, 50);
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_WORLD;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_BUILDING_INTERACT) {
        drawFurnitureInteract();

        const char* actions[4];
        uint8_t numActions = getFurnitureActions(interactingFurniture, actions);

        if (numActions > 0) {
            if (platform_dpad_up_pressed() && menuSelection > 0) {
                menuSelection--;
                platform_beep(NOTE_G4, 30);
            }
            if (platform_dpad_down_pressed() && menuSelection < numActions - 1) {
                menuSelection++;
                platform_beep(NOTE_E4, 30);
            }

            if (platform_button_pressed(BTN_A)) {
                if (executeFurnitureAction(interactingFurniture, menuSelection)) {
                    if (gameState != STATE_WORLD && gameState != STATE_SAVE_SLOTS) {
                        gameState = STATE_BUILDING;
                    }
                    menuSelection = 0;
                }
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_BUILDING;
            menuSelection = 0;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_CHEST_OPENING) {
        drawChestOpening();

        unsigned long now = platform_millis();
        if (now - chestAnimTime >= CHEST_ANIM_DELAY) {
            chestAnimTime = now;
            chestAnimFrame++;

            if (chestAnimFrame == 1) {
                platform_beep(NOTE_E5, 100);
            } else if (chestAnimFrame == 2) {
                platform_beep(NOTE_G5, 100);
            } else if (chestAnimFrame >= 3) {
                addItem(ITEM_SEED, 3);
                addItem(ITEM_HAMMER);
                addItem(ITEM_AXE);
                addItem(ITEM_LASSO);
                setTileUnderPlayer(TG_DIRT);
                processAction();
                gameState = STATE_CHEST_OBTAINED;
                platform_beep(NOTE_G5, 200);
            }
        }

    } else if (gameState == STATE_CHEST_OBTAINED) {
        drawChestObtained();

        if (platform_button_pressed(BTN_A) || platform_button_pressed(BTN_B)) {
            gameState = STATE_WORLD;
            menuSelection = 0;
            platform_beep(NOTE_C5, 50);
        }

    // ========================================================================
    // Save/Load States
    // ========================================================================

    } else if (gameState == STATE_SAVE_SLOTS || gameState == STATE_LOAD_SLOTS) {
        drawSlotBrowser();

        if (platform_dpad_up_pressed() && selectedSlot > 0) {
            selectedSlot--;
            platform_beep(NOTE_G4, 30);
        }
        if (platform_dpad_down_pressed() && selectedSlot < SAVE_SLOT_COUNT - 1) {
            selectedSlot++;
            platform_beep(NOTE_E4, 30);
        }

        if (platform_button_pressed(BTN_A)) {
            if (isSaving) {
                if (saveHeaders[selectedSlot].valid == 0x01) {
                    menuSelection = 0;
                    gameState = STATE_CONFIRM_OVERWRITE;
                    platform_beep(NOTE_E5, 50);
                } else {
                    t9Init("");
                    gameState = STATE_T9_INPUT;
                    platform_beep(NOTE_E5, 50);
                }
            } else {
                if (saveHeaders[selectedSlot].valid == 0x01) {
                    menuSelection = 0;
                    gameState = STATE_LOAD_CONFIRM;
                    platform_beep(NOTE_E5, 50);
                } else {
                    platform_beep(NOTE_C4, 100);
                }
            }
        }

        if (platform_button_pressed(BTN_B)) {
            if (isSaving) {
                gameState = STATE_BUILDING;
            } else {
                gameState = STATE_MENU;
            }
            menuSelection = 0;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_T9_INPUT) {
        drawT9InputScreen();

        if (t9HandleInput()) {
            if (t9InputComplete) {
                const char* name = t9GetText();
                if (strlen(name) == 0) {
                    char defaultName[13];
                    snprintf(defaultName, 13, "SAVE %d", selectedSlot + 1);
                    saveGame(selectedSlot, defaultName, true);
                } else {
                    saveGame(selectedSlot, name, true);
                }
                loadAllSaveHeaders();
                gameState = STATE_SAVE_SUCCESS;
                platform_beep(NOTE_G5, 100);
            } else if (t9InputCancelled) {
                gameState = STATE_SAVE_SLOTS;
                platform_beep(NOTE_C5, 50);
            }
        }

    } else if (gameState == STATE_CONFIRM_OVERWRITE) {
        drawConfirmOverwrite();

        if (platform_dpad_left_pressed() && menuSelection > 0) {
            menuSelection = 0;
            platform_beep(NOTE_G4, 30);
        }
        if (platform_dpad_right_pressed() && menuSelection < 1) {
            menuSelection = 1;
            platform_beep(NOTE_E4, 30);
        }

        if (platform_button_pressed(BTN_A)) {
            if (menuSelection == 0) {
                t9Init(saveHeaders[selectedSlot].name);
                gameState = STATE_T9_INPUT;
                platform_beep(NOTE_E5, 50);
            } else {
                gameState = STATE_SAVE_SLOTS;
                platform_beep(NOTE_C5, 50);
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_SAVE_SLOTS;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_LOAD_CONFIRM) {
        drawLoadConfirm();

        if (platform_dpad_left_pressed() && menuSelection > 0) {
            menuSelection = 0;
            platform_beep(NOTE_G4, 30);
        }
        if (platform_dpad_right_pressed() && menuSelection < 1) {
            menuSelection = 1;
            platform_beep(NOTE_E4, 30);
        }

        if (platform_button_pressed(BTN_A)) {
            if (menuSelection == 0) {
                int8_t wasInBuilding = loadGame(selectedSlot);
                if (wasInBuilding >= 0) {
                    gameState = wasInBuilding ? STATE_BUILDING : STATE_WORLD;
                    platform_play_melody(startupMelody, startupDurations, STARTUP_LEN);
                } else {
                    gameState = STATE_LOAD_SLOTS;
                    platform_beep(NOTE_C4, 200);
                }
            } else {
                gameState = STATE_LOAD_SLOTS;
                platform_beep(NOTE_C5, 50);
            }
        }

        if (platform_button_pressed(BTN_B)) {
            gameState = STATE_LOAD_SLOTS;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_SAVE_SUCCESS) {
        drawSaveSuccess();

        if (platform_button_pressed(BTN_A) || platform_button_pressed(BTN_B)) {
            gameState = STATE_BUILDING;
            menuSelection = 0;
            platform_beep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_DIALOG) {
        // Draw the world behind the dialog
        drawWorldMap();
        // Draw dialog overlay
        drawDialog();

        if (platform_button_pressed(BTN_A)) {
            platform_beep(NOTE_E5, 30);
            if (advanceDialog()) {
                // Dialog complete - check if this was the spirit dialog
                if (progression.spiritSpawned && !progression.spiritDialogComplete) {
                    progression.spiritDialogComplete = true;
                    // Start FOV unlock animation
                    fovAnimFrame = 0;
                    fovAnimTime = currentTime;
                    gameState = STATE_FOV_ANIMATION;
                } else {
                    gameState = STATE_WORLD;
                }
            }
        }

    } else if (gameState == STATE_FOV_ANIMATION) {
        // Blink the hidden columns in and out
        bool showFullFov = (fovAnimFrame % 2) == 1;

        // Temporarily set fovUnlocked for drawing
        bool originalFov = progression.fovUnlocked;
        progression.fovUnlocked = showFullFov;
        drawWorldMap();
        progression.fovUnlocked = originalFov;

        // Update animation
        if (currentTime - fovAnimTime >= FOV_BLINK_DELAY) {
            fovAnimFrame++;
            fovAnimTime = currentTime;

            if (fovAnimFrame >= FOV_BLINK_COUNT) {
                // Animation complete - unlock FOV permanently
                progression.fovUnlocked = true;
                progression.fovAnimationDone = true;
                gameState = STATE_WORLD;
                platform_play_melody(startupMelody, startupDurations, STARTUP_LEN);
            }
        }
    }

    platform_render();
}

#endif // TG_GAME_H
