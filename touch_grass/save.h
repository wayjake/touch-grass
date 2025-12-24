#ifndef TG_SAVE_H
#define TG_SAVE_H

#include "../shared/platform.h"
#include "terrain.h"
#include "inventory.h"
#include "building.h"
#include "progression.h"
#include "creatures.h"

// Only include LittleFS on ESP32 platform
#ifdef ARDUINO
#include <LittleFS.h>
#endif

// Web platform uses EM_ASM to call JavaScript
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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
#ifdef ARDUINO
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
#elif defined(__EMSCRIPTEN__)
    // Load most recent slot from localStorage
    mostRecentSlot = EM_ASM_INT({
        var val = localStorage.getItem('touchgrass_last_slot');
        return val !== null ? parseInt(val, 10) : -1;
    });
    if (mostRecentSlot >= SAVE_SLOT_COUNT) {
        mostRecentSlot = -1;
    }
#endif
    return true;
}

// Read save header only (for browsing)
bool readSaveHeader(uint8_t slot, SaveHeader* header) {
    if (slot >= SAVE_SLOT_COUNT) return false;

#ifdef ARDUINO
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
#elif defined(__EMSCRIPTEN__)
    // Check if slot exists in localStorage
    int exists = EM_ASM_INT({
        var key = 'touchgrass_slot_' + $0;
        return localStorage.getItem(key) !== null ? 1 : 0;
    }, slot);

    if (!exists) {
        header->valid = 0x00;
        header->name[0] = '\0';
        return true;
    }

    // Read header data from localStorage via JavaScript
    header->valid = 0x01;
    header->version = SAVE_VERSION;

    // Get save name
    EM_ASM({
        var key = 'touchgrass_slot_' + $0;
        var saveJson = localStorage.getItem(key);
        if (saveJson) {
            var save = JSON.parse(saveJson);
            var name = save.name || "";
            var maxLen = $2;
            for (var i = 0; i < maxLen && i < name.length; i++) {
                HEAPU8[$1 + i] = name.charCodeAt(i);
            }
            HEAPU8[$1 + Math.min(name.length, maxLen)] = 0;
        }
    }, slot, header->name, SAVE_NAME_MAX);

    // Get timestamp
    header->timestamp = EM_ASM_INT({
        var key = 'touchgrass_slot_' + $0;
        var saveJson = localStorage.getItem(key);
        if (saveJson) {
            var save = JSON.parse(saveJson);
            return save.timestamp || 0;
        }
        return 0;
    }, slot);

    return true;
#else
    header->valid = 0x00;
    header->name[0] = '\0';
    return true;
#endif
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

#ifdef ARDUINO
    File f = LittleFS.open("/saves/last.idx", "w");
    if (!f) return false;

    f.write(slot);
    f.close();
#elif defined(__EMSCRIPTEN__)
    EM_ASM({
        localStorage.setItem("touchgrass_last_slot", $0.toString());
    }, slot);
#endif
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

#ifdef ARDUINO
    // Write to file
    char filename[24];
    getSlotFilename(slot, filename);

    File file = LittleFS.open(filename, "w");
    if (!file) return false;

    size_t written = file.write((uint8_t*)&save, sizeof(SaveSlot));
    file.close();

    if (written != sizeof(SaveSlot)) return false;
#elif defined(__EMSCRIPTEN__)
    // Serialize SaveData as base64 and store in localStorage
    EM_ASM({
        var slotNum = $0;
        var namePtr = $1;
        var timestamp = $2;
        var dataPtr = $3;
        var dataSize = $4;

        // Read name string from WASM memory
        var saveName = "";
        for (var i = 0; i < 13; i++) {
            var c = HEAPU8[namePtr + i];
            if (c === 0) break;
            saveName += String.fromCharCode(c);
        }

        // Read binary SaveData and encode as base64
        var bytes = new Uint8Array(dataSize);
        for (var i = 0; i < dataSize; i++) {
            bytes[i] = HEAPU8[dataPtr + i];
        }
        var binary = "";
        for (var i = 0; i < bytes.length; i++) {
            binary += String.fromCharCode(bytes[i]);
        }
        var dataBase64 = btoa(binary);

        // Manually construct JSON to avoid object literal syntax issues
        var jsonStr = '{"version":1,"name":"' + saveName + '","timestamp":' + timestamp + ',"data":"' + dataBase64 + '"}';

        var key = "touchgrass_slot_" + slotNum;
        localStorage.setItem(key, jsonStr);
    }, slot, name, save.header.timestamp, &save.data, sizeof(SaveData));
#endif

    // Update cached header
    memcpy(&saveHeaders[slot], &save.header, sizeof(SaveHeader));

    // Set as most recent
    setMostRecentSave(slot);

    return true;
}

// Load game from slot, returns wasInBuilding flag
int8_t loadGame(uint8_t slot) {
    if (slot >= SAVE_SLOT_COUNT) return -1;

#ifdef ARDUINO
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
#elif defined(__EMSCRIPTEN__)
    // Load from localStorage
    SaveData loadedData;
    int wasInBuilding = EM_ASM_INT({
        var slot = $0;
        var dataPtr = $1;
        var dataSize = $2;

        var key = "touchgrass_slot_" + slot;
        var saveJson = localStorage.getItem(key);
        if (!saveJson) return -1;

        try {
            var save = JSON.parse(saveJson);
            if (!save.data) return -1;

            // Decode base64 to binary
            var binary = atob(save.data);
            if (binary.length !== dataSize) {
                console.warn("Save data size mismatch:", binary.length, "vs", dataSize);
                return -1;
            }

            // Write to WASM memory
            for (var i = 0; i < binary.length; i++) {
                HEAPU8[dataPtr + i] = binary.charCodeAt(i);
            }

            // Return wasInBuilding flag (last byte of SaveData)
            return HEAPU8[dataPtr + dataSize - 1];
        } catch (e) {
            console.error("Failed to load save:", e);
            return -1;
        }
    }, slot, &loadedData, sizeof(SaveData));

    if (wasInBuilding < 0) return -1;

    // Unpack into game state
    unpackGameState(&loadedData);

    // Update most recent
    setMostRecentSave(slot);

    return wasInBuilding;
#else
    return -1;  // Save not supported
#endif
}

// Delete a save slot
bool deleteSave(uint8_t slot) {
    if (slot >= SAVE_SLOT_COUNT) return false;

#ifdef ARDUINO
    char filename[24];
    getSlotFilename(slot, filename);

    if (LittleFS.exists(filename)) {
        LittleFS.remove(filename);
    }
#elif defined(__EMSCRIPTEN__)
    EM_ASM({
        var key = "touchgrass_slot_" + $0;
        localStorage.removeItem(key);
    }, slot);
#endif

    // Clear cached header
    saveHeaders[slot].valid = 0x00;
    saveHeaders[slot].name[0] = '\0';

    // Update most recent if deleted
    if (mostRecentSlot == slot) {
        mostRecentSlot = -1;
#ifdef ARDUINO
        LittleFS.remove("/saves/last.idx");
#elif defined(__EMSCRIPTEN__)
        EM_ASM({
            localStorage.removeItem("touchgrass_last_slot");
        });
#endif
    }

    return true;
}

#endif
