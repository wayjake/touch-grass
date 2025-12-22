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
    STATE_WORLD,
    STATE_TILE_VIEW,
    STATE_INVENTORY,
    STATE_ITEM_VIEW,
    STATE_GAME_OVER,
    STATE_BUILDING,
    STATE_BUILDING_INTERACT
};

// Game state variables
static GameState gameState = STATE_WORLD;
static uint8_t menuSelection = 0;
static uint8_t inventoryScroll = 0;
static ItemType selectedItem = ITEM_NONE;
static uint8_t inventoryTab = 0;
static char interactingFurniture = TG_FLOOR;

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

static uint8_t getTileActions(char tile, const char** actionNames) {
    switch (tile) {
        case TG_GRASS:
            actionNames[0] = "Cut";
            actionNames[1] = "Examine";
            return 2;
        case TG_DIRT:
            actionNames[0] = "Dig";
            if (hasItem(ITEM_SEED)) {
                actionNames[1] = "Plant";
                if (hasItem(ITEM_HAMMER) && getItemCount(ITEM_WOOD) >= 3 && canBuild4x4()) {
                    actionNames[2] = "Build 4x4";
                    actionNames[3] = "Examine";
                    return 4;
                }
                actionNames[2] = "Examine";
                return 3;
            }
            if (hasItem(ITEM_HAMMER) && getItemCount(ITEM_WOOD) >= 3 && canBuild4x4()) {
                actionNames[1] = "Build 4x4";
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
            actionNames[0] = "Enter";
            actionNames[1] = "Examine";
            return 2;
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
        if (processAction()) return true;
        return false;
    }

    if (tile == TG_TREE && strcmp(actionName, "Cut") == 0) {
        setTileUnderPlayer(TG_DIRT);
        addItem(ITEM_WOOD);
        platform_beep(NOTE_G4, 100);
        platform_beep(NOTE_E4, 100);
        if (processAction()) return true;
        return false;
    }

    if (tile == TG_DIRT && strcmp(actionName, "Dig") == 0) {
        setTileUnderPlayer(TG_WATER);
        addItem(ITEM_DIRT);
        platform_beep(NOTE_C4, 100);
        if (processAction()) return true;
        if (!hasAdjacentLand()) {
            deathReason = DEATH_DROWNED;
            return true;
        }
        return false;
    }

    if (tile == TG_WATER && strcmp(actionName, "Place Dirt") == 0) {
        if (removeItem(ITEM_DIRT)) {
            setTileUnderPlayer(TG_DIRT);
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_C5, 50);
            if (processAction()) return true;
        }
        return false;
    }

    if (tile == TG_DIRT && strcmp(actionName, "Plant") == 0) {
        if (removeItem(ITEM_SEED)) {
            setTileUnderPlayer(TG_SEEDLING);
            platform_beep(NOTE_C5, 50);
            platform_beep(NOTE_E5, 50);
            if (processAction()) return true;
        }
        return false;
    }

    if (tile == TG_DIRT && strcmp(actionName, "Build 4x4") == 0) {
        if (hasItem(ITEM_HAMMER) && canBuild4x4() && removeItem(ITEM_WOOD, 3)) {
            placeBuilding4x4();
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_C5, 50);
            platform_beep(NOTE_E5, 100);
            if (processAction()) return true;
        }
        return false;
    }

    if (tile == TG_CHEST && strcmp(actionName, "Open") == 0) {
        addItem(ITEM_SEED, 3);
        addItem(ITEM_HAMMER);
        addItem(ITEM_AXE);
        setTileUnderPlayer(TG_DIRT);
        platform_beep(NOTE_C5, 100);
        platform_beep(NOTE_E5, 100);
        platform_beep(NOTE_G5, 100);
        if (processAction()) return true;
        return false;
    }

    if (tile == TG_SHRUB && strcmp(actionName, "Harvest") == 0) {
        addItem(ITEM_FRUIT);
        setTileUnderPlayer(TG_SEEDLING);
        platform_beep(NOTE_A4, 50);
        platform_beep(NOTE_C5, 50);
        if (processAction()) return true;
        return false;
    }

    if (tile == TG_BUILDING && strcmp(actionName, "Enter") == 0) {
        current_building_x = tg_playerX;
        current_building_y = tg_playerY;
        initBuildingInterior();
        gameState = STATE_BUILDING;
        platform_beep(NOTE_C5, 50);
        platform_beep(NOTE_E5, 50);
        return true;
    }

    if (strcmp(actionName, "Examine") == 0) {
        platform_beep(NOTE_A4, 80);
        if (processAction()) return true;
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
            if (processAction()) return true;
        }
        return true;
    }

    if (item == ITEM_COOKED_FRUIT && strcmp(actionName, "Eat") == 0) {
        if (removeItem(ITEM_COOKED_FRUIT)) {
            restoreHunger(HUNGER_RESTORE * 2);
            platform_beep(NOTE_E5, 50);
            platform_beep(NOTE_G5, 50);
            if (processAction()) return true;
        }
        return true;
    }

    if (item == ITEM_SEED && strcmp(actionName, "Plant") == 0) {
        if (tg_underPlayer == TG_DIRT && removeItem(ITEM_SEED)) {
            setTileUnderPlayer(TG_SEEDLING);
            platform_beep(NOTE_C5, 50);
            platform_beep(NOTE_E5, 50);
            if (processAction()) return true;
        }
        return true;
    }

    if (item == ITEM_DIRT && strcmp(actionName, "Place") == 0) {
        if (tg_underPlayer == TG_WATER && removeItem(ITEM_DIRT)) {
            setTileUnderPlayer(TG_DIRT);
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_C5, 50);
            if (processAction()) return true;
        }
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
            actionNames[0] = "Use Computer";
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
        if (processAction()) return true;
        return true;
    }

    if (furniture == TG_DESK && strcmp(actionName, "Use Computer") == 0) {
        platform_beep(NOTE_E5, 50);
        platform_beep(NOTE_G5, 50);
        platform_beep(NOTE_E5, 50);
        if (processAction()) return true;
        return true;
    }

    if (furniture == TG_STOVE && strcmp(actionName, "Light Fire") == 0) {
        lightStove();
        platform_beep(NOTE_C5, 100);
        platform_beep(NOTE_E5, 100);
        if (processAction()) return true;
        return true;
    }

    if (furniture == TG_STOVE_LIT && strcmp(actionName, "Cook Fruit") == 0) {
        if (removeItem(ITEM_FRUIT)) {
            addItem(ITEM_COOKED_FRUIT);
            platform_beep(NOTE_G4, 50);
            platform_beep(NOTE_A4, 50);
            platform_beep(NOTE_C5, 100);
            if (processAction()) return true;
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
        if (processAction()) return true;
        return true;
    }

    if (furniture == TG_DOOR && strcmp(actionName, "Exit") == 0) {
        gameState = STATE_WORLD;
        platform_beep(NOTE_C5, 50);
        return true;
    }

    return false;
}

// ============================================================================
// Drawing Functions
// ============================================================================

static void drawWorldMap() {
    platform_set_text_size(1);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == tg_playerX && y == tg_playerY) {
                platform_draw_tile(x, y, TILE_CHAR);
            } else {
                platform_set_cursor(x * PLATFORM_TILE_SIZE, y * PLATFORM_TILE_SIZE);
                platform_print_char(tg_map[y][x]);
            }
        }
    }
}

static void drawScaledSprite(int x, int y, const uint8_t* sprite, int scale) {
    platform_draw_sprite_scaled(x, y, sprite, scale);
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

    platform_set_cursor(0, 56);
    platform_print("A:Do B:Back");
}

static void drawInventoryScreen() {
    platform_set_text_size(1);
    platform_set_cursor(0, 0);
    platform_print("[Inv]");
    platform_set_cursor(40, 0);
    platform_print("  Status");
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
    platform_print("  Inv");
    platform_set_cursor(40, 0);
    platform_print("[Status]");
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

    platform_set_cursor(0, 56);
    platform_print("A:Interact B:Exit");
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
// Main Game Functions
// ============================================================================

inline void game_setup() {
    initInventory();
    generateTerrain();
    platform_play_melody(startupMelody, startupDurations, STARTUP_LEN);
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

    if (gameState == STATE_WORLD) {
        drawWorldMap();

        if (currentTime - lastMoveTime >= MOVE_DELAY) {
            bool moved = false;

            if (platform_dpad_up()) {
                moved = movePlayer(0, -1);
            } else if (platform_dpad_down()) {
                moved = movePlayer(0, 1);
            } else if (platform_dpad_left()) {
                moved = movePlayer(-1, 0);
            } else if (platform_dpad_right()) {
                moved = movePlayer(1, 0);
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
            gameState = STATE_TILE_VIEW;
            menuSelection = 0;
            platform_beep(NOTE_E5, 50);
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
        if (platform_dpad_right_pressed() && inventoryTab < 1) {
            inventoryTab++;
            menuSelection = 0;
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
        } else {
            drawStatusScreen();
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
            initInventory();
            generateTerrain();
            gameState = STATE_WORLD;
            menuSelection = 0;
            platform_led_off();
            platform_play_melody(startupMelody, startupDurations, STARTUP_LEN);
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
            if (building_underPlayer == TG_DOOR) {
                interactingFurniture = TG_DOOR;
                gameState = STATE_BUILDING_INTERACT;
                menuSelection = 0;
                platform_beep(NOTE_E5, 50);
            } else {
                char adjacent = TG_FLOOR;
                if (isAdjacentTo(TG_BED)) adjacent = TG_BED;
                else if (isAdjacentTo(TG_DESK)) adjacent = TG_DESK;
                else if (isAdjacentTo(TG_STOVE_LIT)) adjacent = TG_STOVE_LIT;
                else if (isAdjacentTo(TG_STOVE)) adjacent = TG_STOVE;

                if (adjacent != TG_FLOOR) {
                    interactingFurniture = adjacent;
                    gameState = STATE_BUILDING_INTERACT;
                    menuSelection = 0;
                    platform_beep(NOTE_E5, 50);
                }
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
                    if (gameState != STATE_WORLD) {
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
    }

    platform_render();
}

#endif // TG_GAME_H
