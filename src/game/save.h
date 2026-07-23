#ifndef TG_SAVE_H
#define TG_SAVE_H

#include "../platform.h"
#include "terrain.h"
#include "inventory.h"
#include "building.h"
#include "progression.h"
#include "creatures.h"

// Saves are POSIX files under ~/.touchgrass/saves
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define SAVE_SLOT_COUNT 6
#define SAVE_NAME_MAX 12
#define SAVE_VERSION 1

// Save header (for quick browsing without loading full data)
#pragma pack(push, 1)
typedef struct {
    uint8_t version;           // Save format version
    uint8_t valid;             // 0x00 = empty, 0x01 = valid
    char name[SAVE_NAME_MAX + 1]; // 12 chars + null
    uint32_t timestamp;        // millis() at save time
} SaveHeader;

// Full save data structure
typedef struct {
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

    // Progression state (added for game progression update)
    uint8_t progressionFlags;  // Bitfield for progression bools
    int8_t spiritX;
    int8_t spiritY;
    int16_t homeChunkX;
    int16_t homeChunkY;
    uint8_t homeLocalX;
    uint8_t homeLocalY;

    // Creature state
    uint8_t creatureCount;
    uint8_t creatureData[MAX_CREATURES * 4];  // type, x, y, flags per creature
    int8_t caughtCreatureIdx;
} SaveData;

typedef struct {
    SaveHeader header;
    SaveData data;
} SaveSlot;
#pragma pack(pop)

// Cached headers for menu display
SaveHeader saveHeaders[SAVE_SLOT_COUNT];
bool saveHeadersLoaded = false;
int8_t mostRecentSlot = -1;

// Build absolute path under ~/.touchgrass (falls back to cwd if HOME unset)
static const char* saveDirPath(char* buffer, size_t size) {
    const char* home = getenv("HOME");
    snprintf(buffer, size, "%s/.touchgrass", home ? home : ".");
    return buffer;
}

static const char* saveSlotPath(uint8_t slot, char* buffer, size_t size) {
    char dir[256];
    saveDirPath(dir, sizeof(dir));
    snprintf(buffer, size, "%s/saves/slot%d.sav", dir, slot);
    return buffer;
}

static const char* lastIdxPath(char* buffer, size_t size) {
    char dir[256];
    saveDirPath(dir, sizeof(dir));
    snprintf(buffer, size, "%s/saves/last.idx", dir);
    return buffer;
}

// Initialize save system (call in setup())
bool initSaveSystem() {
    // Create ~/.touchgrass/saves if needed
    char dir[256];
    saveDirPath(dir, sizeof(dir));
    mkdir(dir, 0755);

    char savesDir[280];
    snprintf(savesDir, sizeof(savesDir), "%s/saves", dir);
    mkdir(savesDir, 0755);

    // Load most recent slot index
    char idxPath[300];
    lastIdxPath(idxPath, sizeof(idxPath));
    FILE* f = fopen(idxPath, "rb");
    if (f) {
        int c = fgetc(f);
        fclose(f);
        mostRecentSlot = (c >= 0 && c < SAVE_SLOT_COUNT) ? (int8_t)c : -1;
    }
    return true;
}

// Read save header only (for browsing)
bool readSaveHeader(uint8_t slot, SaveHeader* header) {
    if (slot >= SAVE_SLOT_COUNT) return false;

    char path[300];
    saveSlotPath(slot, path, sizeof(path));

    FILE* file = fopen(path, "rb");
    if (!file) {
        header->valid = 0x00;
        header->name[0] = '\0';
        return true;
    }

    size_t bytesRead = fread(header, 1, sizeof(SaveHeader), file);
    fclose(file);

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

    char idxPath[300];
    lastIdxPath(idxPath, sizeof(idxPath));
    FILE* f = fopen(idxPath, "wb");
    if (!f) return false;

    fputc(slot, f);
    fclose(f);
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

    // Progression state
    data->progressionFlags = packProgressionFlags();
    data->spiritX = progression.spiritX;
    data->spiritY = progression.spiritY;
    data->homeChunkX = progression.homeChunkX;
    data->homeChunkY = progression.homeChunkY;
    data->homeLocalX = progression.homeLocalX;
    data->homeLocalY = progression.homeLocalY;

    // Creature state - initialize to safe defaults
    data->creatureCount = 0;
    data->caughtCreatureIdx = -1;
    memset(data->creatureData, 0, sizeof(data->creatureData));
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

    // Progression state
    unpackProgressionFlags(data->progressionFlags);
    progression.spiritX = data->spiritX;
    progression.spiritY = data->spiritY;
    progression.homeChunkX = data->homeChunkX;
    progression.homeChunkY = data->homeChunkY;
    progression.homeLocalX = data->homeLocalX;
    progression.homeLocalY = data->homeLocalY;

    // Creature state - initialize creatures (will be regenerated based on biome)
    initCreatures();
    spawnCreatures(0, 0);  // Respawn creatures in temperate biome for now
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
    save.header.timestamp = platform_millis();

    // Pack game state
    packGameState(&save.data, inBuilding);

    char path[300];
    saveSlotPath(slot, path, sizeof(path));

    FILE* file = fopen(path, "wb");
    if (!file) return false;

    size_t written = fwrite(&save, 1, sizeof(SaveSlot), file);
    fclose(file);

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

    char path[300];
    saveSlotPath(slot, path, sizeof(path));

    FILE* file = fopen(path, "rb");
    if (!file) return -1;

    SaveSlot save;
    size_t bytesRead = fread(&save, 1, sizeof(SaveSlot), file);
    fclose(file);

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

    char path[300];
    saveSlotPath(slot, path, sizeof(path));
    remove(path);

    // Clear cached header
    saveHeaders[slot].valid = 0x00;
    saveHeaders[slot].name[0] = '\0';

    // Update most recent if deleted
    if (mostRecentSlot == slot) {
        mostRecentSlot = -1;
        char idxPath[300];
        lastIdxPath(idxPath, sizeof(idxPath));
        remove(idxPath);
    }

    return true;
}

#endif
