/*
 * ===============================================================================
 *
 *    _____                _       ____
 *   |_   _|__  _   _  ___| |__   / ___|_ __ __ _ ___ ___
 *     | |/ _ \| | | |/ __| '_ \ | |  _| '__/ _` / __/ __|
 *     | | (_) | |_| | (__| | | || |_| | | | (_| \__ \__ \
 *     |_|\___/ \__,_|\___|_| |_(_)____|_|  \__,_|___/___/
 *
 *   A tile-based exploration game for ESP32 + OLED
 *
 * ===============================================================================
 */

#include "shared/config.h"
#include "shared/hardware.h"
#include "shared/graphics.h"
#include "shared/sound.h"
#include "touch_grass/terrain.h"
#include "touch_grass/inventory.h"
#include "touch_grass/building.h"
#include "touch_grass/t9_dict.h"
#include "touch_grass/t9_input.h"
#include "touch_grass/save.h"

// Game states
enum GameState {
    STATE_SPLASH,          // Title splash screen
    STATE_MENU,            // Main menu
    STATE_WORLD,           // Walking around the map
    STATE_TILE_VIEW,       // Zoomed into a tile
    STATE_INVENTORY,       // Inventory screen
    STATE_ITEM_VIEW,       // Inspecting an item
    STATE_GAME_OVER,       // Death screen
    STATE_BUILDING,        // Inside a building
    STATE_BUILDING_INTERACT, // Interacting with building furniture
    STATE_CHEST_OPENING,   // Chest opening animation
    STATE_CHEST_OBTAINED,  // Showing items obtained from chest
    // Save/Load states
    STATE_COMPUTER_MENU,     // Computer interface menu
    STATE_SAVE_SLOTS,        // Browse save slots for saving
    STATE_LOAD_SLOTS,        // Browse save slots for loading
    STATE_T9_INPUT,          // T9 text entry for naming saves
    STATE_CONFIRM_OVERWRITE, // Confirm overwriting existing save
    STATE_SAVE_SUCCESS,      // Save complete confirmation
    STATE_LOAD_CONFIRM       // Confirm before loading (loses progress)
};

GameState gameState = STATE_SPLASH;
uint8_t menuSelection = 0;
uint8_t inventoryScroll = 0;
ItemType selectedItem = ITEM_NONE;
uint8_t inventoryTab = 0;  // 0 = Inventory, 1 = Status
char interactingFurniture = TG_FLOOR;  // Currently interacting furniture

// Save/Load state variables
uint8_t selectedSlot = 0;           // Currently selected save slot (0-5)
uint8_t slotScroll = 0;             // Scroll offset for slot list
bool isSaving = true;               // true = saving, false = loading
GameState returnAfterT9 = STATE_BUILDING;  // Where to return after T9 input

// Chest animation state
uint8_t chestAnimFrame = 0;
unsigned long chestAnimTime = 0;
const unsigned long CHEST_ANIM_DELAY = 300;  // ms between frames

// Movement cooldown for smooth walking
unsigned long lastMoveTime = 0;
const unsigned long MOVE_DELAY = 150;

// Startup melody
const int tgStartupMelody[] PROGMEM = {
    NOTE_G4, NOTE_C5, NOTE_E5, NOTE_G5
};
const int tgStartupDurations[] PROGMEM = {
    100, 100, 100, 300
};
#define TG_STARTUP_LEN 4

// Death melody
const int tgDeathMelody[] PROGMEM = {
    NOTE_E4, NOTE_D4, NOTE_C4, NOTE_REST
};
const int tgDeathDurations[] PROGMEM = {
    200, 200, 400, 100
};
#define TG_DEATH_LEN 4

// Helper: Process an action (decrements hunger, increments action counter)
// Returns true if player died from starvation
bool processAction() {
    if (decrementHunger()) {
        deathReason = DEATH_STARVED;
        return true;
    }
    if (incrementActionCounter()) {
        growSeedlings();
    }
    return false;
}

// Get actions available for a tile type
uint8_t getTileActions(char tile, const char** actionNames) {
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

// Execute an action on the current tile, returns true if should exit tile view
bool executeTileAction(char tile, uint8_t action) {
    const char* actions[4];
    getTileActions(tile, actions);
    const char* actionName = actions[action];

    // Cut grass
    if (tile == TG_GRASS && strcmp(actionName, "Cut") == 0) {
        setTileUnderPlayer(TG_DIRT);
        playBeep(NOTE_E5, 50);
        processAction();
        gameState = STATE_WORLD;
        return true;
    }

    // Cut tree (requires axe)
    if (tile == TG_TREE && strcmp(actionName, "Cut") == 0) {
        setTileUnderPlayer(TG_DIRT);
        addItem(ITEM_WOOD);
        playBeep(NOTE_G4, 100);
        playBeep(NOTE_E4, 100);
        processAction();
        gameState = STATE_WORLD;
        return true;
    }

    // Dig dirt -> water (with drowning check), get dirt item
    if (tile == TG_DIRT && strcmp(actionName, "Dig") == 0) {
        setTileUnderPlayer(TG_WATER);
        addItem(ITEM_DIRT);
        playBeep(NOTE_C4, 100);
        processAction();
        // Check for drowning
        if (!hasAdjacentLand()) {
            deathReason = DEATH_DROWNED;
            return true;
        }
        gameState = STATE_WORLD;
        return true;
    }

    // Place dirt on water -> becomes dirt tile
    if (tile == TG_WATER && strcmp(actionName, "Place Dirt") == 0) {
        if (removeItem(ITEM_DIRT)) {
            setTileUnderPlayer(TG_DIRT);
            playBeep(NOTE_G4, 50);
            playBeep(NOTE_C5, 50);
            processAction();
            gameState = STATE_WORLD;
            return true;
        }
        return false;
    }

    // Plant seed on dirt
    if (tile == TG_DIRT && strcmp(actionName, "Plant") == 0) {
        if (removeItem(ITEM_SEED)) {
            setTileUnderPlayer(TG_SEEDLING);
            playBeep(NOTE_C5, 50);
            playBeep(NOTE_E5, 50);
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
            playBeep(NOTE_G4, 50);
            playBeep(NOTE_C5, 50);
            processAction();
            gameState = STATE_WORLD;
            return true;
        }
        return false;
    }

    // Open chest - start animation (stays in animation state)
    if (tile == TG_CHEST && strcmp(actionName, "Open") == 0) {
        chestAnimFrame = 0;
        chestAnimTime = millis();
        gameState = STATE_CHEST_OPENING;
        playBeep(NOTE_C5, 100);
        return true;
    }

    // Harvest shrub
    if (tile == TG_SHRUB && strcmp(actionName, "Harvest") == 0) {
        addItem(ITEM_FRUIT);
        setTileUnderPlayer(TG_SEEDLING);
        playBeep(NOTE_A4, 50);
        playBeep(NOTE_C5, 50);
        processAction();
        gameState = STATE_WORLD;
        return true;
    }

    // Enter building (goes to building state) - only if complete 2x2
    if (tile == TG_BUILDING && strcmp(actionName, "Enter") == 0) {
        if (!isPartOfCompleteBuilding(tg_playerX, tg_playerY)) {
            return false;  // Can't enter incomplete building
        }
        current_building_x = tg_playerX;
        current_building_y = tg_playerY;
        initBuildingInterior();
        gameState = STATE_BUILDING;
        playBeep(NOTE_C5, 50);
        playBeep(NOTE_E5, 50);
        return true;
    }

    // Examine - stays in tile view to show info
    if (strcmp(actionName, "Examine") == 0) {
        playBeep(NOTE_A4, 80);
        processAction();
        // Stay in tile view for examine
        return false;
    }

    return false;
}

// Get actions for an item
uint8_t getItemActions(ItemType item, const char** actionNames) {
    switch (item) {
        case ITEM_FRUIT:
            actionNames[0] = "Eat";
            actionNames[1] = "Drop";
            return 2;
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

// Execute item action, returns true if should exit item view
bool executeItemAction(ItemType item, uint8_t action) {
    const char* actions[4];
    getItemActions(item, actions);
    const char* actionName = actions[action];

    // Eat fruit
    if (item == ITEM_FRUIT && strcmp(actionName, "Eat") == 0) {
        if (removeItem(ITEM_FRUIT)) {
            restoreHunger(HUNGER_RESTORE);
            playBeep(NOTE_C5, 50);
            playBeep(NOTE_E5, 50);
            if (processAction()) return true;
        }
        return true; // Exit item view
    }

    // Eat cooked fruit (restores more hunger)
    if (item == ITEM_COOKED_FRUIT && strcmp(actionName, "Eat") == 0) {
        if (removeItem(ITEM_COOKED_FRUIT)) {
            restoreHunger(HUNGER_RESTORE * 2);  // Double hunger restore
            playBeep(NOTE_E5, 50);
            playBeep(NOTE_G5, 50);
            if (processAction()) return true;
        }
        return true;
    }

    // Plant seed
    if (item == ITEM_SEED && strcmp(actionName, "Plant") == 0) {
        if (tg_underPlayer == TG_DIRT && removeItem(ITEM_SEED)) {
            setTileUnderPlayer(TG_SEEDLING);
            playBeep(NOTE_C5, 50);
            playBeep(NOTE_E5, 50);
            if (processAction()) return true;
        }
        return true;
    }

    // Place dirt on water
    if (item == ITEM_DIRT && strcmp(actionName, "Place") == 0) {
        if (tg_underPlayer == TG_WATER && removeItem(ITEM_DIRT)) {
            setTileUnderPlayer(TG_DIRT);
            playBeep(NOTE_G4, 50);
            playBeep(NOTE_C5, 50);
            if (processAction()) return true;
        }
        return true;
    }

    // Drop any item
    if (strcmp(actionName, "Drop") == 0) {
        removeItem(item);
        playBeep(NOTE_E4, 50);
        return true;
    }

    return false;
}

// Check if a 2x2 area starting at (x,y) is all building tiles
bool isComplete2x2At(int x, int y) {
    if (x < 0 || y < 0 || x + 1 >= MAP_WIDTH || y + 1 >= MAP_HEIGHT) return false;
    return tg_map[y][x] == TG_BUILDING &&
           tg_map[y][x+1] == TG_BUILDING &&
           tg_map[y+1][x] == TG_BUILDING &&
           tg_map[y+1][x+1] == TG_BUILDING;
}

// Check if position is part of any complete 2x2 building
bool isPartOfCompleteBuilding(int x, int y) {
    if (tg_map[y][x] != TG_BUILDING) return false;
    // Check all 4 possible 2x2 configurations this tile could be part of
    return isComplete2x2At(x, y) ||      // this is top-left
           isComplete2x2At(x-1, y) ||    // this is top-right
           isComplete2x2At(x, y-1) ||    // this is bottom-left
           isComplete2x2At(x-1, y-1);    // this is bottom-right
}

// Check if position is the top-left corner of a complete 2x2 building (for cabin rendering)
bool isBuildingTopLeft(int x, int y) {
    if (tg_map[y][x] != TG_BUILDING) return false;
    // Must be a complete 2x2 with this as top-left
    if (!isComplete2x2At(x, y)) return false;
    // Check if tile above is NOT building (or out of bounds)
    bool topClear = (y == 0) || (tg_map[y-1][x] != TG_BUILDING);
    // Check if tile to left is NOT building (or out of bounds)
    bool leftClear = (x == 0) || (tg_map[y][x-1] != TG_BUILDING);
    return topClear && leftClear;
}

// Check if position is part of a building but NOT top-left (should skip rendering)
bool isBuildingNonOrigin(int x, int y) {
    if (tg_map[y][x] != TG_BUILDING) return false;
    return !isBuildingTopLeft(x, y);
}

// Draw splash screen
void drawSplashScreen() {
    // Title "TOUCH" and "GRASS" on two lines
    display.setTextSize(2);
    display.setCursor(34, 4);
    display.print("TOUCH");
    display.setCursor(34, 22);
    display.print("GRASS");

    // Draw character sprite scaled 3x to the right of title
    drawScaledSprite(88, 16, TILE_CHAR, 3);

    // "Press A to continue" at bottom
    display.setTextSize(1);
    display.setCursor(14, 54);
    display.print("Press A to continue");
}

// Get number of main menu options based on save state
uint8_t getMainMenuOptionCount() {
    uint8_t count = 1;  // NEW GAME always present
    if (getMostRecentSave() >= 0) count++;  // CONTINUE
    if (anySaveExists()) count++;  // LOAD
    return count;
}

// Get main menu option name by index
const char* getMainMenuOption(uint8_t idx) {
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

// Draw main menu
void drawMainMenu() {
    display.setTextSize(2);
    display.setCursor(28, 4);
    display.print("- MENU -");

    display.setTextSize(1);
    display.drawLine(0, 22, 127, 22, SH110X_WHITE);

    // Menu options
    uint8_t optionCount = getMainMenuOptionCount();
    int menuY = 28;

    for (uint8_t i = 0; i < optionCount && i < 3; i++) {
        display.setCursor(20, menuY + i * 10);
        if (menuSelection == i) {
            display.print("> ");
        } else {
            display.print("  ");
        }
        display.print(getMainMenuOption(i));
    }

    display.setCursor(0, 56);
    display.print("A:Select");
}

// Draw the world map
void drawWorldMap() {
    display.setTextSize(1);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            char tile = tg_map[y][x];

            // Always draw player on their tile (even on buildings)
            if (x == tg_playerX && y == tg_playerY) {
                drawTile(x, y, TILE_CHAR);
                continue;
            }

            // Skip non-origin building tiles (covered by 16x16 cabin)
            if (isBuildingNonOrigin(x, y)) {
                continue;
            }

            if (tile == TG_CHEST) {
                // Draw chest as sprite
                drawTile(x, y, TILE_CHEST);
            } else if (isBuildingTopLeft(x, y)) {
                // Draw 16x16 cabin at building origin
                drawTile16(x * TILE_SIZE, y * TILE_SIZE, TILE_CABIN_16);
            } else if (tile == TG_BUILDING) {
                // Draw incomplete/single building tile as sprite
                drawTile(x, y, TILE_BUILDING);
            } else {
                // Draw other tiles as text
                display.setCursor(x * TILE_SIZE, y * TILE_SIZE);
                display.print(tile);
            }
        }
    }
}

// Draw scaled sprite helper
void drawScaledSprite(int x, int y, const uint8_t* sprite, int scale) {
    for (int row = 0; row < 8; row++) {
        uint8_t rowData = pgm_read_byte(&sprite[row]);
        for (int col = 0; col < 8; col++) {
            if (rowData & (1 << (7 - col))) {
                display.fillRect(x + col * scale, y + row * scale, scale, scale, SH110X_WHITE);
            }
        }
    }
}

// Draw the tile detail view
void drawTileView() {
    char tile = tg_underPlayer;
    const char* tileName = getTileName(tile);
    const uint8_t* sprite = getTileSprite(tile);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(tileName);

    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    drawScaledSprite(8, 16, sprite, 3);

    const char* actions[4];
    uint8_t numActions = getTileActions(tile, actions);

    int menuX = 50;
    int menuY = 16;
    const uint8_t maxVisible = 3;  // Max items visible at once

    display.setCursor(menuX, menuY);
    display.print("Actions:");

    // Calculate scroll offset to keep selection visible
    uint8_t scrollOffset = 0;
    if (menuSelection >= maxVisible) {
        scrollOffset = menuSelection - maxVisible + 1;
    }

    // Draw visible items
    for (uint8_t i = 0; i < maxVisible && (i + scrollOffset) < numActions; i++) {
        uint8_t itemIdx = i + scrollOffset;
        display.setCursor(menuX, menuY + 12 + i * 10);
        if (itemIdx == menuSelection) {
            display.print("> ");
        } else {
            display.print("  ");
        }
        display.print(actions[itemIdx]);
    }

    // Show scroll indicators if needed
    if (scrollOffset > 0) {
        display.setCursor(menuX + 70, menuY + 12);
        display.print("^");
    }
    if (scrollOffset + maxVisible < numActions) {
        display.setCursor(menuX + 70, menuY + 12 + (maxVisible - 1) * 10);
        display.print("v");
    }

    display.setCursor(0, 56);
    display.print("A:Do B:Back");
}

// Draw chest opening animation
void drawChestOpening() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Chest");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    // Center the 16x16 chest sprite
    int spriteX = 56;  // Centered on 128px width
    int spriteY = 24;  // Upper middle area

    // Select frame based on animation state
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

    display.setCursor(40, 50);
    display.print("Opening...");
}

// Draw items obtained from chest
void drawChestObtained() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("You obtained:");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    // Draw the open chest
    drawTile16(8, 18, TILE_CHEST_OPEN_16);

    // List items obtained
    int textX = 32;
    int textY = 18;

    display.setCursor(textX, textY);
    display.print("* 3 Seeds");
    display.setCursor(textX, textY + 10);
    display.print("* Hammer");
    display.setCursor(textX, textY + 20);
    display.print("* Axe");

    display.setCursor(20, 56);
    display.print("Press A or B");
}

// Draw inventory screen
void drawInventoryScreen() {
    display.setTextSize(1);

    // Tab header
    display.setCursor(0, 0);
    display.print("[Inv]");
    display.setCursor(40, 0);
    display.print("  Status");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    uint8_t filledSlots = countFilledSlots();

    if (filledSlots == 0) {
        display.setCursor(20, 28);
        display.print("(empty)");
    } else {
        int menuY = 14;
        uint8_t displayCount = min((uint8_t)4, filledSlots);

        for (uint8_t i = 0; i < displayCount; i++) {
            uint8_t idx = i + inventoryScroll;
            if (idx >= filledSlots) break;

            int8_t slot = getNthFilledSlot(idx);
            if (slot < 0) continue;

            display.setCursor(0, menuY + i * 10);
            if (idx == menuSelection) {
                display.print("> ");
            } else {
                display.print("  ");
            }
            display.print(getItemName(inventory[slot].type));
            if (inventory[slot].count > 1) {
                display.print(" x");
                display.print(inventory[slot].count);
            }
        }

        // Scroll indicators
        if (inventoryScroll > 0) {
            display.setCursor(120, 14);
            display.print("^");
        }
        if (inventoryScroll + 4 < filledSlots) {
            display.setCursor(120, 44);
            display.print("v");
        }
    }

    display.setCursor(0, 56);
    display.print("</>:Tab A:Use B:Back");
}

// Draw status screen
void drawStatusScreen() {
    display.setTextSize(1);

    // Tab header
    display.setCursor(0, 0);
    display.print("  Inv");
    display.setCursor(40, 0);
    display.print("[Status]");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    // Hunger display
    display.setCursor(0, 16);
    display.print("Hunger: ");
    display.print(playerHunger);
    display.print("/250");

    // Hunger bar
    int barWidth = 80;
    int barHeight = 6;
    int barX = 10;
    int barY = 28;
    int fillWidth = (playerHunger * barWidth) / 250;
    display.drawRect(barX, barY, barWidth, barHeight, SH110X_WHITE);
    display.fillRect(barX, barY, fillWidth, barHeight, SH110X_WHITE);

    // Game time (actions)
    display.setCursor(0, 40);
    display.print("Actions: ");
    display.print(totalActions);

    display.setCursor(0, 56);
    display.print("</>:Tab B:Back");
}

// Draw item view screen
void drawItemView() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(getItemName(selectedItem));
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    const char* actions[4];
    uint8_t numActions = getItemActions(selectedItem, actions);

    int menuX = 10;
    int menuY = 18;

    for (uint8_t i = 0; i < numActions; i++) {
        display.setCursor(menuX, menuY + i * 10);
        if (i == menuSelection) {
            display.print("> ");
        } else {
            display.print("  ");
        }
        display.print(actions[i]);
    }

    display.setCursor(0, 56);
    display.print("A:Do B:Back");
}

// Draw game over screen
void drawGameOver() {
    display.setTextSize(1);

    display.setCursor(32, 10);
    display.print("GAME OVER");

    display.setCursor(20, 30);
    if (deathReason == DEATH_DROWNED) {
        display.print("You drowned!");
    } else if (deathReason == DEATH_STARVED) {
        display.print("You starved!");
    }

    display.setCursor(12, 50);
    display.print("Press A to restart");
}

// Draw building interior
void drawBuildingInterior() {
    display.setTextSize(1);

    // Title at top
    display.setCursor(30, 0);
    display.print("~ Home ~");

    // Calculate offset to center the interior (8x7 with walls)
    int offsetX = (128 - (INTERIOR_WIDTH + 2) * 8) / 2;
    int offsetY = 10;

    // Draw walls (border)
    for (int x = 0; x < INTERIOR_WIDTH + 2; x++) {
        drawScaledSprite(offsetX + x * 8, offsetY, TILE_WALL, 1);
        drawScaledSprite(offsetX + x * 8, offsetY + (INTERIOR_HEIGHT + 1) * 8, TILE_WALL, 1);
    }
    for (int y = 1; y < INTERIOR_HEIGHT + 1; y++) {
        drawScaledSprite(offsetX, offsetY + y * 8, TILE_WALL, 1);
        drawScaledSprite(offsetX + (INTERIOR_WIDTH + 1) * 8, offsetY + y * 8, TILE_WALL, 1);
    }

    // Draw interior tiles
    for (int y = 0; y < INTERIOR_HEIGHT; y++) {
        for (int x = 0; x < INTERIOR_WIDTH; x++) {
            int screenX = offsetX + (x + 1) * 8;
            int screenY = offsetY + (y + 1) * 8;

            if (x == building_playerX && y == building_playerY) {
                // Draw player
                drawScaledSprite(screenX, screenY, TILE_CHAR, 1);
            } else {
                // Draw tile
                drawScaledSprite(screenX, screenY, getTileSprite(building_map[y][x]), 1);
            }
        }
    }

}

// ============================================================================
// Save/Load UI Functions
// ============================================================================

// Draw computer menu
void drawComputerMenu() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Computer");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    // Draw scaled sprite
    drawScaledSprite(8, 16, TILE_DESK, 3);

    // Menu
    int menuX = 50;
    int menuY = 16;
    display.setCursor(menuX, menuY);
    display.print("Options:");

    display.setCursor(menuX, menuY + 12);
    if (menuSelection == 0) {
        display.print("> Save Game");
    } else {
        display.print("  Save Game");
    }

    display.setCursor(0, 56);
    display.print("A:Select B:Back");
}

// Draw save/load slot browser
void drawSlotBrowser() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    if (isSaving) {
        display.print("SAVE GAME");
    } else {
        display.print("LOAD GAME");
    }
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    // Ensure headers are loaded
    if (!saveHeadersLoaded) {
        loadAllSaveHeaders();
    }

    // Show 4 slots at a time
    const uint8_t maxVisible = 4;
    if (selectedSlot >= slotScroll + maxVisible) {
        slotScroll = selectedSlot - maxVisible + 1;
    } else if (selectedSlot < slotScroll) {
        slotScroll = selectedSlot;
    }

    int menuY = 14;
    for (uint8_t i = 0; i < maxVisible && (i + slotScroll) < SAVE_SLOT_COUNT; i++) {
        uint8_t slotIdx = i + slotScroll;
        display.setCursor(0, menuY + i * 10);

        if (slotIdx == selectedSlot) {
            display.print(">");
        } else {
            display.print(" ");
        }

        display.print(slotIdx + 1);
        display.print(". ");

        if (saveHeaders[slotIdx].valid == 0x01) {
            display.print(saveHeaders[slotIdx].name);
        } else {
            display.print("[empty]");
        }
    }

    // Scroll indicators
    if (slotScroll > 0) {
        display.setCursor(120, 14);
        display.print("^");
    }
    if (slotScroll + maxVisible < SAVE_SLOT_COUNT) {
        display.setCursor(120, 44);
        display.print("v");
    }

    display.setCursor(0, 56);
    display.print("A:Select B:Back");
}

// Draw T9 input screen with phone keypad
void drawT9InputScreen() {
    display.setTextSize(1);

    // Draw current text with cursor at top
    display.setCursor(0, 0);
    const char* text = t9GetText();
    display.print(text);

    // Show pending character with underscore
    char pending = t9GetPendingChar();
    if (pending) {
        display.print(pending);
    }
    display.print("_");

    display.drawLine(0, 9, 127, 9, SH110X_WHITE);

    // Draw phone keypad grid on left side
    // Grid starts at x=0, y=12
    // Each key cell is 14x12 pixels
    const int gridX = 0;
    const int gridY = 12;
    const int cellW = 14;
    const int cellH = 12;

    // Key labels for display
    const char* keyLabels[4][3] = {
        {"1", "2", "3"},
        {"4", "5", "6"},
        {"7", "8", "9"},
        {"<", "0", "OK"}
    };
    const char* keyLetters[4][3] = {
        {".,!?", "ABC", "DEF"},
        {"GHI", "JKL", "MNO"},
        {"PQR", "TUV", "WXY"},
        {"DEL", " ", ""}
    };

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 3; col++) {
            int x = gridX + col * cellW;
            int y = gridY + row * cellH;

            // Highlight selected cell
            if (col == t9GridX && row == t9GridY) {
                display.fillRect(x, y, cellW - 1, cellH - 1, SH110X_WHITE);
                display.setTextColor(SH110X_BLACK);
            } else {
                display.drawRect(x, y, cellW - 1, cellH - 1, SH110X_WHITE);
                display.setTextColor(SH110X_WHITE);
            }

            // Draw key number/label centered
            display.setCursor(x + 4, y + 2);
            display.print(keyLabels[row][col]);
        }
    }
    display.setTextColor(SH110X_WHITE);

    // Show letters for selected key on right side
    int8_t selectedKey = t9GetSelectedKey();
    display.setCursor(46, 12);
    if (selectedKey >= 0 && selectedKey <= 9) {
        display.print("[");
        display.print(selectedKey);
        display.print("] ");
        display.print(T9_KEYS[selectedKey]);
    } else if (selectedKey == -1) {
        display.print("DELETE");
    } else if (selectedKey == -2) {
        display.print("CONFIRM");
    }

    // Show predictions on right side (if any)
    if (t9PredictionCount > 0 && !t9InPredictionMode) {
        display.setCursor(46, 24);
        display.print("Suggest:");
        char wordBuf[13];
        for (uint8_t i = 0; i < t9PredictionCount && i < 2; i++) {
            display.setCursor(46, 34 + i * 10);
            copyDictWord(t9Predictions[i], wordBuf, 10);
            display.print(wordBuf);
        }
    } else if (t9InPredictionMode) {
        display.setCursor(46, 24);
        display.print("Pick word:");
        char wordBuf[13];
        for (uint8_t i = 0; i < t9PredictionCount && i < 3; i++) {
            display.setCursor(46, 34 + i * 10);
            if (i == t9PredictionIndex) {
                display.print(">");
            } else {
                display.print(" ");
            }
            copyDictWord(t9Predictions[i], wordBuf, 9);
            display.print(wordBuf);
        }
    }

    // Bottom hints
    display.setCursor(0, 56);
    display.print("A:Type B:Space/Cancel");
}

// Draw confirm overwrite dialog
void drawConfirmOverwrite() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Overwrite Save?");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    display.setCursor(0, 16);
    display.print("Slot ");
    display.print(selectedSlot + 1);
    display.print(": ");
    display.print(saveHeaders[selectedSlot].name);

    display.setCursor(0, 30);
    display.print("will be overwritten.");

    display.setCursor(10, 44);
    if (menuSelection == 0) {
        display.print("> Yes   No");
    } else {
        display.print("  Yes > No");
    }

    display.setCursor(0, 56);
    display.print("A:Select B:Cancel");
}

// Draw load confirm dialog
void drawLoadConfirm() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Load Game?");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    display.setCursor(0, 16);
    display.print("Load: ");
    display.print(saveHeaders[selectedSlot].name);

    display.setCursor(0, 30);
    display.print("Current progress");
    display.setCursor(0, 38);
    display.print("will be lost!");

    display.setCursor(10, 48);
    if (menuSelection == 0) {
        display.print("> Yes   No");
    } else {
        display.print("  Yes > No");
    }

    display.setCursor(0, 56);
    display.print("A:Select B:Cancel");
}

// Draw save success message
void drawSaveSuccess() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Game Saved!");
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    display.setCursor(0, 24);
    display.print("Saved to slot ");
    display.print(selectedSlot + 1);

    display.setCursor(0, 36);
    display.print("Name: ");
    display.print(saveHeaders[selectedSlot].name);

    display.setCursor(0, 56);
    display.print("A:Continue");
}

// ============================================================================

// Get furniture actions
uint8_t getFurnitureActions(char furniture, const char** actionNames) {
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

// Draw furniture interaction menu
void drawFurnitureInteract() {
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(getInteriorTileName(interactingFurniture));
    display.drawLine(0, 10, 127, 10, SH110X_WHITE);

    // Draw scaled sprite
    drawScaledSprite(8, 16, getTileSprite(interactingFurniture), 3);

    const char* actions[4];
    uint8_t numActions = getFurnitureActions(interactingFurniture, actions);

    int menuX = 50;
    int menuY = 16;

    if (numActions > 0) {
        display.setCursor(menuX, menuY);
        display.print("Actions:");

        for (uint8_t i = 0; i < numActions; i++) {
            display.setCursor(menuX, menuY + 12 + i * 10);
            if (i == menuSelection) {
                display.print("> ");
            } else {
                display.print("  ");
            }
            display.print(actions[i]);
        }
    }

    display.setCursor(0, 56);
    display.print("A:Do B:Back");
}

// Execute furniture action
bool executeFurnitureAction(char furniture, uint8_t action) {
    const char* actions[4];
    getFurnitureActions(furniture, actions);
    if (action >= getFurnitureActions(furniture, actions)) return false;
    const char* actionName = actions[action];

    // Bed - Rest (small hunger restore)
    if (furniture == TG_BED && strcmp(actionName, "Rest") == 0) {
        restoreHunger(10);
        playBeep(NOTE_C4, 200);
        if (processAction()) return true;
        return true;
    }

    // Computer - Save Game (opens save slot browser)
    if (furniture == TG_DESK && strcmp(actionName, "Save Game") == 0) {
        playBeep(NOTE_E5, 50);
        isSaving = true;
        selectedSlot = 0;
        slotScroll = 0;
        loadAllSaveHeaders();
        gameState = STATE_SAVE_SLOTS;
        return false;  // Don't return to building, stay in save flow
    }

    // Stove - Light fire
    if (furniture == TG_STOVE && strcmp(actionName, "Light Fire") == 0) {
        lightStove();
        playBeep(NOTE_C5, 100);
        playBeep(NOTE_E5, 100);
        if (processAction()) return true;
        return true;
    }

    // Lit stove - Cook fruit
    if (furniture == TG_STOVE_LIT && strcmp(actionName, "Cook Fruit") == 0) {
        if (removeItem(ITEM_FRUIT)) {
            addItem(ITEM_COOKED_FRUIT);
            playBeep(NOTE_G4, 50);
            playBeep(NOTE_A4, 50);
            playBeep(NOTE_C5, 100);
            if (processAction()) return true;
        }
        return true;
    }

    // Lit stove - Extinguish
    if (furniture == TG_STOVE_LIT && strcmp(actionName, "Extinguish") == 0) {
        stove_lit = false;
        // Update map
        for (int y = 0; y < INTERIOR_HEIGHT; y++) {
            for (int x = 0; x < INTERIOR_WIDTH; x++) {
                if (building_map[y][x] == TG_STOVE_LIT) {
                    building_map[y][x] = TG_STOVE;
                }
            }
        }
        playBeep(NOTE_E4, 100);
        if (processAction()) return true;
        return true;
    }

    // Door - Exit
    if (furniture == TG_DOOR && strcmp(actionName, "Exit") == 0) {
        gameState = STATE_WORLD;
        playBeep(NOTE_C5, 50);
        return true;
    }

    return false;
}

void setup() {
    hardware_init();
    initSaveSystem();
    // Game initialization happens when selecting NEW GAME from menu
}

void loop() {
    button_update();
    display.clearDisplay();

    unsigned long currentTime = millis();

    // Check for death
    if (deathReason != DEATH_NONE && gameState != STATE_GAME_OVER) {
        gameState = STATE_GAME_OVER;
        playMelody(tgDeathMelody, tgDeathDurations, TG_DEATH_LEN);
        led_set(255, 0, 0);
    }

    if (gameState == STATE_SPLASH) {
        drawSplashScreen();

        if (button_pressed(BUTTON_A)) {
            gameState = STATE_MENU;
            menuSelection = 0;
            playBeep(NOTE_E5, 50);
        }

    } else if (gameState == STATE_MENU) {
        drawMainMenu();

        uint8_t optionCount = getMainMenuOptionCount();

        // Navigate menu
        if (dpad_up_pressed() && menuSelection > 0) {
            menuSelection--;
            playBeep(NOTE_G4, 30);
        }
        if (dpad_down_pressed() && menuSelection < optionCount - 1) {
            menuSelection++;
            playBeep(NOTE_E4, 30);
        }

        if (button_pressed(BUTTON_A)) {
            const char* option = getMainMenuOption(menuSelection);
            if (strcmp(option, "NEW GAME") == 0) {
                initInventory();
                generateTerrain();
                gameState = STATE_WORLD;
                playMelody(tgStartupMelody, tgStartupDurations, TG_STARTUP_LEN);
            } else if (strcmp(option, "CONTINUE") == 0) {
                int8_t slot = getMostRecentSave();
                if (slot >= 0) {
                    int8_t wasInBuilding = loadGame(slot);
                    if (wasInBuilding >= 0) {
                        gameState = wasInBuilding ? STATE_BUILDING : STATE_WORLD;
                        playMelody(tgStartupMelody, tgStartupDurations, TG_STARTUP_LEN);
                    }
                }
            } else if (strcmp(option, "LOAD") == 0) {
                isSaving = false;
                selectedSlot = 0;
                slotScroll = 0;
                loadAllSaveHeaders();
                gameState = STATE_LOAD_SLOTS;
                playBeep(NOTE_E5, 50);
            }
        }

        if (button_pressed(BUTTON_B)) {
            gameState = STATE_SPLASH;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_WORLD) {
        drawWorldMap();

        // Movement with cooldown
        if (currentTime - lastMoveTime >= MOVE_DELAY) {
            bool moved = false;

            if (dpad_up()) {
                moved = movePlayer(0, -1);
            } else if (dpad_down()) {
                moved = movePlayer(0, 1);
            } else if (dpad_left()) {
                moved = movePlayer(-1, 0);
            } else if (dpad_right()) {
                moved = movePlayer(1, 0);
            }

            if (moved) {
                playBeep(NOTE_C5, 20);
                lastMoveTime = currentTime;
                // Movement costs hunger
                if (decrementHunger()) {
                    deathReason = DEATH_STARVED;
                }
                if (incrementActionCounter()) {
                    growSeedlings();
                }
            }
        }

        // A button opens tile view
        if (button_pressed(BUTTON_A)) {
            gameState = STATE_TILE_VIEW;
            menuSelection = 0;
            playBeep(NOTE_E5, 50);
        }

        // B button opens inventory
        if (button_pressed(BUTTON_B)) {
            gameState = STATE_INVENTORY;
            menuSelection = 0;
            inventoryScroll = 0;
            inventoryTab = 0;
            playBeep(NOTE_G4, 50);
        }

    } else if (gameState == STATE_TILE_VIEW) {
        drawTileView();

        const char* actions[4];
        uint8_t numActions = getTileActions(tg_underPlayer, actions);

        if (dpad_up_pressed() && menuSelection > 0) {
            menuSelection--;
            playBeep(NOTE_G4, 30);
        }
        if (dpad_down_pressed() && menuSelection < numActions - 1) {
            menuSelection++;
            playBeep(NOTE_E4, 30);
        }

        if (button_pressed(BUTTON_A)) {
            executeTileAction(tg_underPlayer, menuSelection);
        }

        if (button_pressed(BUTTON_B)) {
            gameState = STATE_WORLD;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_INVENTORY) {
        // Tab switching with left/right
        if (dpad_left_pressed() && inventoryTab > 0) {
            inventoryTab--;
            menuSelection = 0;
            inventoryScroll = 0;
            playBeep(NOTE_C5, 30);
        }
        if (dpad_right_pressed() && inventoryTab < 1) {
            inventoryTab++;
            menuSelection = 0;
            playBeep(NOTE_C5, 30);
        }

        if (inventoryTab == 0) {
            // Inventory tab
            drawInventoryScreen();

            uint8_t filledSlots = countFilledSlots();

            if (filledSlots > 0) {
                if (dpad_up_pressed() && menuSelection > 0) {
                    menuSelection--;
                    if (menuSelection < inventoryScroll) {
                        inventoryScroll = menuSelection;
                    }
                    playBeep(NOTE_G4, 30);
                }
                if (dpad_down_pressed() && menuSelection < filledSlots - 1) {
                    menuSelection++;
                    if (menuSelection >= inventoryScroll + 4) {
                        inventoryScroll = menuSelection - 3;
                    }
                    playBeep(NOTE_E4, 30);
                }

                if (button_pressed(BUTTON_A)) {
                    int8_t slot = getNthFilledSlot(menuSelection);
                    if (slot >= 0) {
                        selectedItem = inventory[slot].type;
                        gameState = STATE_ITEM_VIEW;
                        menuSelection = 0;
                        playBeep(NOTE_E5, 50);
                    }
                }
            }
        } else {
            // Status tab
            drawStatusScreen();
        }

        if (button_pressed(BUTTON_B)) {
            gameState = STATE_WORLD;
            inventoryTab = 0;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_ITEM_VIEW) {
        drawItemView();

        const char* actions[4];
        uint8_t numActions = getItemActions(selectedItem, actions);

        if (dpad_up_pressed() && menuSelection > 0) {
            menuSelection--;
            playBeep(NOTE_G4, 30);
        }
        if (dpad_down_pressed() && menuSelection < numActions - 1) {
            menuSelection++;
            playBeep(NOTE_E4, 30);
        }

        if (button_pressed(BUTTON_A)) {
            if (executeItemAction(selectedItem, menuSelection)) {
                // Return to inventory after action
                gameState = STATE_INVENTORY;
                menuSelection = 0;
                inventoryScroll = 0;
            }
        }

        if (button_pressed(BUTTON_B)) {
            gameState = STATE_INVENTORY;
            menuSelection = 0;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_GAME_OVER) {
        drawGameOver();

        if (button_pressed(BUTTON_A)) {
            // Return to menu
            deathReason = DEATH_NONE;
            gameState = STATE_MENU;
            menuSelection = 0;
            led_off();
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_BUILDING) {
        drawBuildingInterior();

        unsigned long currentTime = millis();

        // Movement inside building
        if (currentTime - lastMoveTime >= MOVE_DELAY) {
            bool moved = false;

            if (dpad_up()) {
                moved = movePlayerInterior(0, -1);
            } else if (dpad_down()) {
                moved = movePlayerInterior(0, 1);
            } else if (dpad_left()) {
                moved = movePlayerInterior(-1, 0);
            } else if (dpad_right()) {
                moved = movePlayerInterior(1, 0);
            }

            if (moved) {
                playBeep(NOTE_C5, 20);
                lastMoveTime = currentTime;
                if (decrementHunger()) {
                    deathReason = DEATH_STARVED;
                }
                if (incrementActionCounter()) {
                    growSeedlings();
                }
            }
        }

        // A button - interact with furniture (standing on or adjacent)
        if (button_pressed(BUTTON_A)) {
            char furniture = TG_FLOOR;

            // First check if standing on interactable furniture
            if (building_underPlayer == TG_DOOR ||
                building_underPlayer == TG_BED ||
                building_underPlayer == TG_DESK ||
                building_underPlayer == TG_STOVE ||
                building_underPlayer == TG_STOVE_LIT) {
                furniture = building_underPlayer;
            } else {
                // Fall back to checking adjacent tiles
                if (isAdjacentTo(TG_BED)) furniture = TG_BED;
                else if (isAdjacentTo(TG_DESK)) furniture = TG_DESK;
                else if (isAdjacentTo(TG_STOVE_LIT)) furniture = TG_STOVE_LIT;
                else if (isAdjacentTo(TG_STOVE)) furniture = TG_STOVE;
            }

            if (furniture != TG_FLOOR) {
                interactingFurniture = furniture;
                gameState = STATE_BUILDING_INTERACT;
                menuSelection = 0;
                playBeep(NOTE_E5, 50);
            }
        }

        // B button - exit building
        if (button_pressed(BUTTON_B)) {
            gameState = STATE_WORLD;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_BUILDING_INTERACT) {
        drawFurnitureInteract();

        const char* actions[4];
        uint8_t numActions = getFurnitureActions(interactingFurniture, actions);

        if (numActions > 0) {
            if (dpad_up_pressed() && menuSelection > 0) {
                menuSelection--;
                playBeep(NOTE_G4, 30);
            }
            if (dpad_down_pressed() && menuSelection < numActions - 1) {
                menuSelection++;
                playBeep(NOTE_E4, 30);
            }

            if (button_pressed(BUTTON_A)) {
                if (executeFurnitureAction(interactingFurniture, menuSelection)) {
                    if (gameState != STATE_WORLD) {
                        gameState = STATE_BUILDING;
                    }
                    menuSelection = 0;
                }
            }
        }

        if (button_pressed(BUTTON_B)) {
            gameState = STATE_BUILDING;
            menuSelection = 0;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_CHEST_OPENING) {
        drawChestOpening();

        // Advance animation frame
        unsigned long now = millis();
        if (now - chestAnimTime >= CHEST_ANIM_DELAY) {
            chestAnimTime = now;
            chestAnimFrame++;

            if (chestAnimFrame == 1) {
                playBeep(NOTE_E5, 100);
            } else if (chestAnimFrame == 2) {
                playBeep(NOTE_G5, 100);
            } else if (chestAnimFrame >= 3) {
                // Animation complete - add items and show obtained screen
                addItem(ITEM_SEED, 3);
                addItem(ITEM_HAMMER);
                addItem(ITEM_AXE);
                setTileUnderPlayer(TG_DIRT);
                processAction();
                gameState = STATE_CHEST_OBTAINED;
                playBeep(NOTE_G5, 200);
            }
        }

    } else if (gameState == STATE_CHEST_OBTAINED) {
        drawChestObtained();

        // Wait for A or B to return to world
        if (button_pressed(BUTTON_A) || button_pressed(BUTTON_B)) {
            gameState = STATE_WORLD;
            menuSelection = 0;
            playBeep(NOTE_C5, 50);
        }

    // ========================================================================
    // Save/Load States
    // ========================================================================

    } else if (gameState == STATE_SAVE_SLOTS || gameState == STATE_LOAD_SLOTS) {
        drawSlotBrowser();

        // Navigate slots
        if (dpad_up_pressed() && selectedSlot > 0) {
            selectedSlot--;
            playBeep(NOTE_G4, 30);
        }
        if (dpad_down_pressed() && selectedSlot < SAVE_SLOT_COUNT - 1) {
            selectedSlot++;
            playBeep(NOTE_E4, 30);
        }

        if (button_pressed(BUTTON_A)) {
            if (isSaving) {
                // Saving - check if slot is occupied
                if (saveHeaders[selectedSlot].valid == 0x01) {
                    // Slot has data - confirm overwrite
                    menuSelection = 0;
                    gameState = STATE_CONFIRM_OVERWRITE;
                    playBeep(NOTE_E5, 50);
                } else {
                    // Empty slot - go to T9 input
                    t9Init("");
                    gameState = STATE_T9_INPUT;
                    playBeep(NOTE_E5, 50);
                }
            } else {
                // Loading - only allow if slot is valid
                if (saveHeaders[selectedSlot].valid == 0x01) {
                    menuSelection = 0;
                    gameState = STATE_LOAD_CONFIRM;
                    playBeep(NOTE_E5, 50);
                } else {
                    // Empty slot - beep error
                    playBeep(NOTE_C4, 100);
                }
            }
        }

        if (button_pressed(BUTTON_B)) {
            if (isSaving) {
                gameState = STATE_BUILDING;
            } else {
                gameState = STATE_MENU;
            }
            menuSelection = 0;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_T9_INPUT) {
        drawT9InputScreen();

        // T9 handles its own input
        if (t9HandleInput()) {
            if (t9InputComplete) {
                // Save with entered name
                const char* name = t9GetText();
                if (strlen(name) == 0) {
                    // Default name if empty
                    char defaultName[13];
                    snprintf(defaultName, 13, "SAVE %d", selectedSlot + 1);
                    saveGame(selectedSlot, defaultName, true);
                } else {
                    saveGame(selectedSlot, name, true);
                }
                loadAllSaveHeaders();  // Refresh headers
                gameState = STATE_SAVE_SUCCESS;
                playBeep(NOTE_G5, 100);
            } else if (t9InputCancelled) {
                gameState = STATE_SAVE_SLOTS;
                playBeep(NOTE_C5, 50);
            }
        }

    } else if (gameState == STATE_CONFIRM_OVERWRITE) {
        drawConfirmOverwrite();

        // Yes/No selection
        if (dpad_left_pressed() && menuSelection > 0) {
            menuSelection = 0;
            playBeep(NOTE_G4, 30);
        }
        if (dpad_right_pressed() && menuSelection < 1) {
            menuSelection = 1;
            playBeep(NOTE_E4, 30);
        }

        if (button_pressed(BUTTON_A)) {
            if (menuSelection == 0) {
                // Yes - go to T9 input with existing name
                t9Init(saveHeaders[selectedSlot].name);
                gameState = STATE_T9_INPUT;
                playBeep(NOTE_E5, 50);
            } else {
                // No - back to slot browser
                gameState = STATE_SAVE_SLOTS;
                playBeep(NOTE_C5, 50);
            }
        }

        if (button_pressed(BUTTON_B)) {
            gameState = STATE_SAVE_SLOTS;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_LOAD_CONFIRM) {
        drawLoadConfirm();

        // Yes/No selection
        if (dpad_left_pressed() && menuSelection > 0) {
            menuSelection = 0;
            playBeep(NOTE_G4, 30);
        }
        if (dpad_right_pressed() && menuSelection < 1) {
            menuSelection = 1;
            playBeep(NOTE_E4, 30);
        }

        if (button_pressed(BUTTON_A)) {
            if (menuSelection == 0) {
                // Yes - load game
                int8_t wasInBuilding = loadGame(selectedSlot);
                if (wasInBuilding >= 0) {
                    gameState = wasInBuilding ? STATE_BUILDING : STATE_WORLD;
                    playMelody(tgStartupMelody, tgStartupDurations, TG_STARTUP_LEN);
                } else {
                    // Load failed
                    gameState = STATE_LOAD_SLOTS;
                    playBeep(NOTE_C4, 200);
                }
            } else {
                // No - back to slot browser
                gameState = STATE_LOAD_SLOTS;
                playBeep(NOTE_C5, 50);
            }
        }

        if (button_pressed(BUTTON_B)) {
            gameState = STATE_LOAD_SLOTS;
            playBeep(NOTE_C5, 50);
        }

    } else if (gameState == STATE_SAVE_SUCCESS) {
        drawSaveSuccess();

        if (button_pressed(BUTTON_A) || button_pressed(BUTTON_B)) {
            gameState = STATE_BUILDING;
            menuSelection = 0;
            playBeep(NOTE_C5, 50);
        }
    }

    display.display();
}
