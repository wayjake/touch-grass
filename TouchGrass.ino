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

// Game states
enum GameState {
    STATE_WORLD,           // Walking around the map
    STATE_TILE_VIEW,       // Zoomed into a tile
    STATE_INVENTORY,       // Inventory screen
    STATE_ITEM_VIEW,       // Inspecting an item
    STATE_GAME_OVER,       // Death screen
    STATE_BUILDING,        // Inside a building
    STATE_BUILDING_INTERACT // Interacting with building furniture
};

GameState gameState = STATE_WORLD;
uint8_t menuSelection = 0;
uint8_t inventoryScroll = 0;
ItemType selectedItem = ITEM_NONE;
uint8_t inventoryTab = 0;  // 0 = Inventory, 1 = Status
char interactingFurniture = TG_FLOOR;  // Currently interacting furniture

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

// Execute an action on the current tile, returns true if should exit tile view
bool executeTileAction(char tile, uint8_t action) {
    const char* actions[4];
    getTileActions(tile, actions);
    const char* actionName = actions[action];

    // Cut grass
    if (tile == TG_GRASS && strcmp(actionName, "Cut") == 0) {
        setTileUnderPlayer(TG_DIRT);
        playBeep(NOTE_E5, 50);
        if (processAction()) return true;
        return false;
    }

    // Cut tree (requires axe)
    if (tile == TG_TREE && strcmp(actionName, "Cut") == 0) {
        setTileUnderPlayer(TG_DIRT);
        addItem(ITEM_WOOD);
        playBeep(NOTE_G4, 100);
        playBeep(NOTE_E4, 100);
        if (processAction()) return true;
        return false;
    }

    // Dig dirt -> water (with drowning check), get dirt item
    if (tile == TG_DIRT && strcmp(actionName, "Dig") == 0) {
        setTileUnderPlayer(TG_WATER);
        addItem(ITEM_DIRT);
        playBeep(NOTE_C4, 100);
        if (processAction()) return true;
        // Check for drowning
        if (!hasAdjacentLand()) {
            deathReason = DEATH_DROWNED;
            return true;
        }
        return false;
    }

    // Place dirt on water -> becomes dirt tile
    if (tile == TG_WATER && strcmp(actionName, "Place Dirt") == 0) {
        if (removeItem(ITEM_DIRT)) {
            setTileUnderPlayer(TG_DIRT);
            playBeep(NOTE_G4, 50);
            playBeep(NOTE_C5, 50);
            if (processAction()) return true;
        }
        return false;
    }

    // Plant seed on dirt
    if (tile == TG_DIRT && strcmp(actionName, "Plant") == 0) {
        if (removeItem(ITEM_SEED)) {
            setTileUnderPlayer(TG_SEEDLING);
            playBeep(NOTE_C5, 50);
            playBeep(NOTE_E5, 50);
            if (processAction()) return true;
        }
        return false;
    }

    // Build 4x4 on dirt
    if (tile == TG_DIRT && strcmp(actionName, "Build 4x4") == 0) {
        if (hasItem(ITEM_HAMMER) && canBuild4x4() && removeItem(ITEM_WOOD, 3)) {
            placeBuilding4x4();
            playBeep(NOTE_G4, 50);
            playBeep(NOTE_C5, 50);
            playBeep(NOTE_E5, 100);
            if (processAction()) return true;
        }
        return false;
    }

    // Open chest
    if (tile == TG_CHEST && strcmp(actionName, "Open") == 0) {
        addItem(ITEM_SEED, 3);
        addItem(ITEM_HAMMER);
        addItem(ITEM_AXE);
        setTileUnderPlayer(TG_DIRT);
        playBeep(NOTE_C5, 100);
        playBeep(NOTE_E5, 100);
        playBeep(NOTE_G5, 100);
        if (processAction()) return true;
        return false;
    }

    // Harvest shrub
    if (tile == TG_SHRUB && strcmp(actionName, "Harvest") == 0) {
        addItem(ITEM_FRUIT);
        setTileUnderPlayer(TG_SEEDLING);
        playBeep(NOTE_A4, 50);
        playBeep(NOTE_C5, 50);
        if (processAction()) return true;
        return false;
    }

    // Enter building
    if (tile == TG_BUILDING && strcmp(actionName, "Enter") == 0) {
        current_building_x = tg_playerX;
        current_building_y = tg_playerY;
        initBuildingInterior();
        gameState = STATE_BUILDING;
        playBeep(NOTE_C5, 50);
        playBeep(NOTE_E5, 50);
        return true;  // Exit tile view
    }

    // Examine (any tile)
    if (strcmp(actionName, "Examine") == 0) {
        playBeep(NOTE_A4, 80);
        if (processAction()) return true;
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

// Draw the world map
void drawWorldMap() {
    display.setTextSize(1);

    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            if (x == tg_playerX && y == tg_playerY) {
                drawTile(x, y, TILE_CHAR);
            } else {
                display.setCursor(x * TILE_SIZE, y * TILE_SIZE);
                display.print(tg_map[y][x]);
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

    display.setCursor(0, 56);
    display.print("A:Do B:Back");
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

    // Instructions at bottom
    display.setCursor(0, 56);
    display.print("A:Interact B:Exit");
}

// Get furniture actions
uint8_t getFurnitureActions(char furniture, const char** actionNames) {
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

    // Computer - Use (just a beep for now)
    if (furniture == TG_DESK && strcmp(actionName, "Use Computer") == 0) {
        playBeep(NOTE_E5, 50);
        playBeep(NOTE_G5, 50);
        playBeep(NOTE_E5, 50);
        if (processAction()) return true;
        return true;
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
    initInventory();
    generateTerrain();
    playMelody(tgStartupMelody, tgStartupDurations, TG_STARTUP_LEN);
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

    if (gameState == STATE_WORLD) {
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
            // Restart game
            initInventory();
            generateTerrain();
            gameState = STATE_WORLD;
            menuSelection = 0;
            led_off();
            playMelody(tgStartupMelody, tgStartupDurations, TG_STARTUP_LEN);
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

        // A button - interact with adjacent furniture
        if (button_pressed(BUTTON_A)) {
            // Check if standing on door
            if (building_underPlayer == TG_DOOR) {
                interactingFurniture = TG_DOOR;
                gameState = STATE_BUILDING_INTERACT;
                menuSelection = 0;
                playBeep(NOTE_E5, 50);
            } else {
                // Check adjacent tiles for furniture
                char adjacent = TG_FLOOR;
                if (isAdjacentTo(TG_BED)) adjacent = TG_BED;
                else if (isAdjacentTo(TG_DESK)) adjacent = TG_DESK;
                else if (isAdjacentTo(TG_STOVE_LIT)) adjacent = TG_STOVE_LIT;
                else if (isAdjacentTo(TG_STOVE)) adjacent = TG_STOVE;

                if (adjacent != TG_FLOOR) {
                    interactingFurniture = adjacent;
                    gameState = STATE_BUILDING_INTERACT;
                    menuSelection = 0;
                    playBeep(NOTE_E5, 50);
                }
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
    }

    display.display();
}
