#ifndef TG_SPRITES_H
#define TG_SPRITES_H

#include <Arduino.h>

// 8x8 pixel tile sprites stored in PROGMEM
// Each byte is one row, MSB-first (bit 7 = leftmost pixel)

// Grass tile - vertical blades pattern
const uint8_t TILE_GRASS[8] PROGMEM = {
    0b00100100,  //   #  #
    0b01001010,  //  #  # #
    0b00100100,  //   #  #
    0b10010010,  // #  #  #
    0b00100100,  //   #  #
    0b01001001,  //  #  #  #
    0b00100100,  //   #  #
    0b10010010   // #  #  #
};

// Water tile - wavy horizontal lines
const uint8_t TILE_WATER[8] PROGMEM = {
    0b00000000,  //
    0b01100110,  //  ##  ##
    0b10011001,  // #  ##  #
    0b00000000,  //
    0b00000000,  //
    0b10011001,  // #  ##  #
    0b01100110,  //  ##  ##
    0b00000000   //
};

// Dirt tile - sparse dots
const uint8_t TILE_DIRT[8] PROGMEM = {
    0b00000000,  //
    0b00100000,  //   #
    0b00000100,  //      #
    0b10000000,  // #
    0b00000010,  //       #
    0b00010000,  //    #
    0b01000000,  //  #
    0b00001000   //     #
};

// Character tile - simple stick figure
const uint8_t TILE_CHAR[8] PROGMEM = {
    0b00111100,  //   ####
    0b00100100,  //   #  #
    0b00111100,  //   ####    (head)
    0b01111110,  //  ######   (arms + body)
    0b00011000,  //    ##     (body)
    0b00011000,  //    ##     (body)
    0b00100100,  //   #  #    (legs)
    0b01100110   //  ##  ##   (feet)
};

// Tree tile - trunk with foliage
const uint8_t TILE_TREE[8] PROGMEM = {
    0b00111100,  //   ####
    0b01111110,  //  ######
    0b11111111,  // ########
    0b01111110,  //  ######
    0b00111100,  //   ####
    0b00011000,  //    ##
    0b00011000,  //    ##
    0b00111100   //   ####    (roots)
};

// Chest tile - treasure box
const uint8_t TILE_CHEST[8] PROGMEM = {
    0b00000000,  //
    0b01111110,  //  ######
    0b11111111,  // ########
    0b10011001,  // #  ##  #  (lock)
    0b11111111,  // ########
    0b11111111,  // ########
    0b11111111,  // ########
    0b00000000   //
};

// Building tile - simple house
const uint8_t TILE_BUILDING[8] PROGMEM = {
    0b00011000,  //    ##     (roof peak)
    0b00111100,  //   ####
    0b01111110,  //  ######
    0b11111111,  // ########
    0b11011011,  // ## ## ##  (windows)
    0b11011011,  // ## ## ##
    0b11000011,  // ##    ##  (door)
    0b11000011   // ##    ##
};

// Shrub tile - berry bush
const uint8_t TILE_SHRUB[8] PROGMEM = {
    0b00100100,  //   #  #
    0b01011010,  //  # ## #
    0b10111101,  // # #### #
    0b01111110,  //  ######
    0b10111101,  // # #### #
    0b01111110,  //  ######
    0b00111100,  //   ####
    0b00011000   //    ##
};

// Seedling tile - small sprout
const uint8_t TILE_SEEDLING[8] PROGMEM = {
    0b00000000,  //
    0b00000000,  //
    0b00010000,  //    #
    0b00111000,  //   ###
    0b00010000,  //    #
    0b00010000,  //    #
    0b00111000,  //   ###     (soil)
    0b00000000   //
};

// Interior tile sprites

// Floor tile - simple dot pattern
const uint8_t TILE_FLOOR[8] PROGMEM = {
    0b00000000,  //
    0b00000000,  //
    0b00000000,  //
    0b00011000,  //    ##
    0b00011000,  //    ##
    0b00000000,  //
    0b00000000,  //
    0b00000000   //
};

// Bed tile - pillow and blanket
const uint8_t TILE_BED[8] PROGMEM = {
    0b01111110,  //  ######
    0b01000010,  //  #    #
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110   //  ######
};

// Desk tile - table with computer
const uint8_t TILE_DESK[8] PROGMEM = {
    0b00111100,  //   ####    (monitor)
    0b00111100,  //   ####
    0b00011000,  //    ##     (stand)
    0b01111110,  //  ######   (desk top)
    0b01000010,  //  #    #
    0b01000010,  //  #    #
    0b01000010,  //  #    #
    0b01000010   //  #    #   (legs)
};

// Stove tile - no fire
const uint8_t TILE_STOVE[8] PROGMEM = {
    0b01111110,  //  ######
    0b01000010,  //  #    #
    0b01011010,  //  # ## #   (burners)
    0b01000010,  //  #    #
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110   //  ######
};

// Stove tile - with fire
const uint8_t TILE_STOVE_FIRE[8] PROGMEM = {
    0b00100100,  //   #  #    (flames)
    0b01011010,  //  # ## #
    0b00100100,  //   #  #
    0b01011010,  //  # ## #   (burners)
    0b01000010,  //  #    #
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110   //  ######
};

// Door tile
const uint8_t TILE_DOOR[8] PROGMEM = {
    0b00111100,  //   ####
    0b00100100,  //   #  #
    0b00100100,  //   #  #
    0b00101100,  //   # ##    (handle)
    0b00100100,  //   #  #
    0b00100100,  //   #  #
    0b00100100,  //   #  #
    0b00111100   //   ####
};

// Wall tile - for building border
const uint8_t TILE_WALL[8] PROGMEM = {
    0b11111111,  // ########
    0b10000001,  // #      #
    0b11111111,  // ########
    0b10000001,  // #      #
    0b11111111,  // ########
    0b10000001,  // #      #
    0b11111111,  // ########
    0b10000001   // #      #
};

// Tile type definitions - World tiles
#define TG_GRASS    'g'
#define TG_WATER    'w'
#define TG_DIRT     'd'
#define TG_CHAR     'c'
#define TG_TREE     't'
#define TG_CHEST    'x'
#define TG_BUILDING 'b'
#define TG_SHRUB    's'
#define TG_SEEDLING 'p'

// Interior tile types
#define TG_FLOOR      '.'
#define TG_BED        'B'
#define TG_DESK       'D'
#define TG_STOVE      'S'
#define TG_STOVE_LIT  'F'
#define TG_DOOR       'O'
#define TG_WALL       '#'

// Get sprite data for a tile type
const uint8_t* getTileSprite(char tileType) {
    switch (tileType) {
        case TG_WATER:    return TILE_WATER;
        case TG_DIRT:     return TILE_DIRT;
        case TG_CHAR:     return TILE_CHAR;
        case TG_TREE:     return TILE_TREE;
        case TG_CHEST:    return TILE_CHEST;
        case TG_BUILDING: return TILE_BUILDING;
        case TG_SHRUB:    return TILE_SHRUB;
        case TG_SEEDLING: return TILE_SEEDLING;
        // Interior tiles
        case TG_FLOOR:    return TILE_FLOOR;
        case TG_BED:      return TILE_BED;
        case TG_DESK:     return TILE_DESK;
        case TG_STOVE:    return TILE_STOVE;
        case TG_STOVE_LIT:return TILE_STOVE_FIRE;
        case TG_DOOR:     return TILE_DOOR;
        case TG_WALL:     return TILE_WALL;
        case TG_GRASS:
        default:          return TILE_GRASS;
    }
}

#endif
