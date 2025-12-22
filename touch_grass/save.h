#ifndef TG_SAVE_H
#define TG_SAVE_H

#include <Arduino.h>
#include <LittleFS.h>
#include "terrain.h"
#include "inventory.h"
#include "building.h"

#define SAVE_SLOT_COUNT 6
#define SAVE_NAME_MAX 12
#define SAVE_VERSION 1

// Save header (for quick browsing without loading full data)
#pragma pack(push, 1)
struct SaveHeader {
    uint8_t version;           // Save format version
    uint8_t valid;             // 0x00 = empty, 0x01 = valid
    char name[SAVE_NAME_MAX + 1]; // 12 chars + null
    uint32_t timestamp;        // millis() at save time
};

// Full save data structure
struct SaveData {
    // Terrain state
    char tg_map[MAP_HEIGHT][MAP_WIDTH];  // 128 bytes
    uint8_t playerX;
    uint8_t playerY;
    char underPlayer;
    uint16_t hunger;
    uint8_t actionCount;
    uint16_t totalActions;
    uint8_t death;  // DeathReason enum

    // Inventory state (12 slots x 2 bytes)
    uint8_t inventoryData[24];

    // Building state
    char buildingMap[INTERIOR_HEIGHT][INTERIOR_WIDTH];  // 30 bytes
    uint8_t buildingPlayerX;
    uint8_t buildingPlayerY;
    char buildingUnderPlayer;
    uint8_t stoveLit;
    uint8_t currentBuildingX;
    uint8_t currentBuildingY;

    // Context
    uint8_t wasInBuilding;  // Was player inside building when saved?
};

struct SaveSlot {
    SaveHeader header;
    SaveData data;
};
#pragma pack(pop)

// Cached headers for menu display
SaveHeader saveHeaders[SAVE_SLOT_COUNT];
bool saveHeadersLoaded = false;
int8_t mostRecentSlot = -1;

// Get filename for a slot
const char* getSlotFilename(uint8_t slot, char* buffer) {
    snprintf(buffer, 24, "/saves/slot%d.sav", slot);
    return buffer;
}

// Initialize save system (call in setup())
bool initSaveSystem() {
    if (!LittleFS.begin(true)) {  // true = format if mount fails
        return false;
    }

    // Create saves directory if needed
    if (!LittleFS.exists("/saves")) {
        LittleFS.mkdir("/saves");
    }

    // Load most recent slot index
    if (LittleFS.exists("/saves/last.idx")) {
        File f = LittleFS.open("/saves/last.idx", "r");
        if (f) {
            mostRecentSlot = f.read();
            f.close();
            if (mostRecentSlot >= SAVE_SLOT_COUNT) {
                mostRecentSlot = -1;
            }
        }
    }

    return true;
}

// Read save header only (for browsing)
bool readSaveHeader(uint8_t slot, SaveHeader* header) {
    if (slot >= SAVE_SLOT_COUNT) return false;

    char filename[24];
    getSlotFilename(slot, filename);

    if (!LittleFS.exists(filename)) {
        header->valid = 0x00;
        header->name[0] = '\0';
        return true;
    }

    File file = LittleFS.open(filename, "r");
    if (!file) {
        header->valid = 0x00;
        return false;
    }

    size_t bytesRead = file.read((uint8_t*)header, sizeof(SaveHeader));
    file.close();

    return bytesRead == sizeof(SaveHeader);
}

// Load all save headers (for slot browser)
void loadAllSaveHeaders() {
    for (uint8_t i = 0; i < SAVE_SLOT_COUNT; i++) {
        readSaveHeader(i, &saveHeaders[i]);
    }
    saveHeadersLoaded = true;
}

// Check if slot is valid
bool isSlotValid(uint8_t slot) {
    if (slot >= SAVE_SLOT_COUNT) return false;
    if (!saveHeadersLoaded) loadAllSaveHeaders();
    return saveHeaders[slot].valid == 0x01;
}

// Check if any save exists
bool anySaveExists() {
    if (!saveHeadersLoaded) loadAllSaveHeaders();
    for (uint8_t i = 0; i < SAVE_SLOT_COUNT; i++) {
        if (saveHeaders[i].valid == 0x01) return true;
    }
    return false;
}

// Get most recent save slot (-1 if none)
int8_t getMostRecentSave() {
    if (mostRecentSlot >= 0 && isSlotValid(mostRecentSlot)) {
        return mostRecentSlot;
    }
    return -1;
}

// Set most recent save slot
bool setMostRecentSave(uint8_t slot) {
    if (slot >= SAVE_SLOT_COUNT) return false;

    File f = LittleFS.open("/saves/last.idx", "w");
    if (!f) return false;

    f.write(slot);
    f.close();
    mostRecentSlot = slot;
    return true;
}

// Pack current game state into SaveData
void packGameState(SaveData* data, bool inBuilding) {
    // Terrain state
    memcpy(data->tg_map, tg_map, sizeof(tg_map));
    data->playerX = tg_playerX;
    data->playerY = tg_playerY;
    data->underPlayer = tg_underPlayer;
    data->hunger = playerHunger;
    data->actionCount = globalActionCount;
    data->totalActions = totalActions;
    data->death = (uint8_t)deathReason;

    // Inventory - pack into byte array
    for (uint8_t i = 0; i < 12; i++) {
        data->inventoryData[i * 2] = (uint8_t)inventory[i].type;
        data->inventoryData[i * 2 + 1] = inventory[i].count;
    }

    // Building state
    memcpy(data->buildingMap, building_map, sizeof(building_map));
    data->buildingPlayerX = building_playerX;
    data->buildingPlayerY = building_playerY;
    data->buildingUnderPlayer = building_underPlayer;
    data->stoveLit = stove_lit ? 1 : 0;
    data->currentBuildingX = current_building_x;
    data->currentBuildingY = current_building_y;

    data->wasInBuilding = inBuilding ? 1 : 0;
}

// Unpack SaveData into game state
void unpackGameState(const SaveData* data) {
    // Terrain state
    memcpy(tg_map, data->tg_map, sizeof(tg_map));
    tg_playerX = data->playerX;
    tg_playerY = data->playerY;
    tg_underPlayer = data->underPlayer;
    playerHunger = data->hunger;
    globalActionCount = data->actionCount;
    totalActions = data->totalActions;
    deathReason = (DeathReason)data->death;

    // Inventory - unpack from byte array
    for (uint8_t i = 0; i < 12; i++) {
        inventory[i].type = (ItemType)data->inventoryData[i * 2];
        inventory[i].count = data->inventoryData[i * 2 + 1];
    }

    // Building state
    memcpy(building_map, data->buildingMap, sizeof(building_map));
    building_playerX = data->buildingPlayerX;
    building_playerY = data->buildingPlayerY;
    building_underPlayer = data->buildingUnderPlayer;
    stove_lit = data->stoveLit != 0;
    current_building_x = data->currentBuildingX;
    current_building_y = data->currentBuildingY;
}

// Save current game to slot
bool saveGame(uint8_t slot, const char* name, bool inBuilding) {
    if (slot >= SAVE_SLOT_COUNT) return false;

    SaveSlot save;

    // Fill header
    save.header.version = SAVE_VERSION;
    save.header.valid = 0x01;
    strncpy(save.header.name, name, SAVE_NAME_MAX);
    save.header.name[SAVE_NAME_MAX] = '\0';
    save.header.timestamp = millis();

    // Pack game state
    packGameState(&save.data, inBuilding);

    // Write to file
    char filename[24];
    getSlotFilename(slot, filename);

    File file = LittleFS.open(filename, "w");
    if (!file) return false;

    size_t written = file.write((uint8_t*)&save, sizeof(SaveSlot));
    file.close();

    if (written != sizeof(SaveSlot)) return false;

    // Update cached header
    memcpy(&saveHeaders[slot], &save.header, sizeof(SaveHeader));

    // Set as most recent
    setMostRecentSave(slot);

    return true;
}

// Load game from slot, returns wasInBuilding flag
int8_t loadGame(uint8_t slot) {
    if (slot >= SAVE_SLOT_COUNT) return -1;

    char filename[24];
    getSlotFilename(slot, filename);

    if (!LittleFS.exists(filename)) return -1;

    File file = LittleFS.open(filename, "r");
    if (!file) return -1;

    SaveSlot save;
    size_t bytesRead = file.read((uint8_t*)&save, sizeof(SaveSlot));
    file.close();

    if (bytesRead != sizeof(SaveSlot)) return -1;
    if (save.header.valid != 0x01) return -1;
    if (save.header.version != SAVE_VERSION) return -1;

    // Unpack into game state
    unpackGameState(&save.data);

    // Update most recent
    setMostRecentSave(slot);

    return save.data.wasInBuilding;
}

// Delete a save slot
bool deleteSave(uint8_t slot) {
    if (slot >= SAVE_SLOT_COUNT) return false;

    char filename[24];
    getSlotFilename(slot, filename);

    if (LittleFS.exists(filename)) {
        LittleFS.remove(filename);
    }

    // Clear cached header
    saveHeaders[slot].valid = 0x00;
    saveHeaders[slot].name[0] = '\0';

    // Update most recent if deleted
    if (mostRecentSlot == slot) {
        mostRecentSlot = -1;
        LittleFS.remove("/saves/last.idx");
    }

    return true;
}

#endif
