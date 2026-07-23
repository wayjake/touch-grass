#ifndef TG_CONFIG_H
#define TG_CONFIG_H

#include "../platform.h"

// Tile system
#define TILE_SIZE       PLATFORM_TILE_SIZE
#define MAP_WIDTH       (PLATFORM_SCREEN_WIDTH / TILE_SIZE)   // 16 tiles
#define MAP_HEIGHT      (PLATFORM_SCREEN_HEIGHT / TILE_SIZE)  // 8 tiles

// Game constants
#define MAX_HUNGER      250
#define HUNGER_RESTORE  25      // Amount restored by eating fruit
#define ACTIONS_TO_GROW 35      // Actions before seedlings become shrubs

#endif
