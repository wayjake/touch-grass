#ifndef TG_INVENTORY_H
#define TG_INVENTORY_H

#include <Arduino.h>

// Item types
enum ItemType {
    ITEM_NONE = 0,
    ITEM_SEED,
    ITEM_FRUIT,
    ITEM_COOKED_FRUIT,
    ITEM_WOOD,
    ITEM_HAMMER,
    ITEM_AXE,
    ITEM_DIRT,
    ITEM_LASSO
};

#define ITEM_TYPE_COUNT 9

// Item names for display
const char* const itemNames[ITEM_TYPE_COUNT] = {
    "Empty", "Seed", "Fruit", "Cooked Fruit", "Wood", "Hammer", "Axe", "Dirt", "Lasso"
};

// Inventory slot structure
struct InventorySlot {
    ItemType type;
    uint8_t count;
};

// Inventory storage
#define MAX_INVENTORY_SLOTS 12
InventorySlot inventory[MAX_INVENTORY_SLOTS];
uint8_t inventorySize = 0;

// Initialize inventory (call in setup)
void initInventory() {
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        inventory[i].type = ITEM_NONE;
        inventory[i].count = 0;
    }
    inventorySize = 0;
}

// Find slot containing item type, returns -1 if not found
int8_t findItemSlot(ItemType item) {
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inventory[i].type == item && inventory[i].count > 0) {
            return i;
        }
    }
    return -1;
}

// Find empty slot, returns -1 if inventory full
int8_t findEmptySlot() {
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inventory[i].type == ITEM_NONE || inventory[i].count == 0) {
            return i;
        }
    }
    return -1;
}

// Add item to inventory, returns true if successful
bool addItem(ItemType item, uint8_t count = 1) {
    if (item == ITEM_NONE) return false;

    // Tools (hammer, axe) don't stack - check if we already have one
    if (item == ITEM_HAMMER || item == ITEM_AXE) {
        if (findItemSlot(item) >= 0) {
            return false; // Already have this tool
        }
        int8_t slot = findEmptySlot();
        if (slot < 0) return false;
        inventory[slot].type = item;
        inventory[slot].count = 1;
        inventorySize++;
        return true;
    }

    // Stackable items - try to add to existing stack
    int8_t existingSlot = findItemSlot(item);
    if (existingSlot >= 0) {
        if (inventory[existingSlot].count + count <= 99) {
            inventory[existingSlot].count += count;
            return true;
        }
    }

    // Create new stack
    int8_t emptySlot = findEmptySlot();
    if (emptySlot < 0) return false;

    inventory[emptySlot].type = item;
    inventory[emptySlot].count = count;
    inventorySize++;
    return true;
}

// Remove item from inventory, returns true if successful
bool removeItem(ItemType item, uint8_t count = 1) {
    int8_t slot = findItemSlot(item);
    if (slot < 0) return false;

    if (inventory[slot].count < count) return false;

    inventory[slot].count -= count;
    if (inventory[slot].count == 0) {
        inventory[slot].type = ITEM_NONE;
        inventorySize--;
    }
    return true;
}

// Get count of specific item type
uint8_t getItemCount(ItemType item) {
    int8_t slot = findItemSlot(item);
    if (slot < 0) return 0;
    return inventory[slot].count;
}

// Check if player has at least one of item
bool hasItem(ItemType item) {
    return findItemSlot(item) >= 0;
}

// Get item name
const char* getItemName(ItemType item) {
    if ((int)item >= 0 && (int)item < ITEM_TYPE_COUNT) {
        return itemNames[item];
    }
    return "Unknown";
}

// Count number of non-empty slots (for display)
uint8_t countFilledSlots() {
    uint8_t count = 0;
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inventory[i].type != ITEM_NONE && inventory[i].count > 0) {
            count++;
        }
    }
    return count;
}

// Get the nth non-empty slot (for inventory display iteration)
// Returns slot index, or -1 if n exceeds filled slots
int8_t getNthFilledSlot(uint8_t n) {
    uint8_t count = 0;
    for (int i = 0; i < MAX_INVENTORY_SLOTS; i++) {
        if (inventory[i].type != ITEM_NONE && inventory[i].count > 0) {
            if (count == n) return i;
            count++;
        }
    }
    return -1;
}

#endif
