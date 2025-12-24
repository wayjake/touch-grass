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

// Chest tile - simple treasure box (8x8 for world map)
const uint8_t TILE_CHEST[8] PROGMEM = {
    0b00000000,  //
    0b01111110,  //  ######   (lid)
    0b11111111,  // ########
    0b11100111,  // ###  ###  (keyhole)
    0b11111111,  // ########
    0b11111111,  // ########
    0b01111110,  //  ######
    0b00000000   //
};

// Chest animation sprites (16x16 for tile view)
// Frame 1: Closed chest with ornate details
const uint8_t TILE_CHEST_CLOSED_16[32] PROGMEM = {
    0b00000000, 0b00000000,  //
    0b01000000, 0b00000010,  //  #            #   (sparkles)
    0b00011111, 0b11111000,  //    ##########
    0b00111111, 0b11111100,  //   ############
    0b01111111, 0b11111110,  //  ##############
    0b01100110, 0b01100110,  //  ##  ##  ##  ##   (decorative bands)
    0b01111111, 0b11111110,  //  ##############
    0b01111001, 0b10011110,  //  ####  ##  ####   (lock plate)
    0b01111011, 0b11011110,  //  #### #### ####   (keyhole)
    0b01111001, 0b10011110,  //  ####  ##  ####
    0b01111111, 0b11111110,  //  ##############
    0b01100110, 0b01100110,  //  ##  ##  ##  ##   (decorative bands)
    0b01111111, 0b11111110,  //  ##############
    0b00111111, 0b11111100,  //   ############
    0b00001100, 0b00110000,  //     ##    ##      (feet)
    0b00000000, 0b00000000   //
};

// Frame 2: Chest opening - lid tilted back
const uint8_t TILE_CHEST_OPENING_16[32] PROGMEM = {
    0b00111111, 0b11111100,  //   ############    (lid tilting)
    0b00011111, 0b11111000,  //    ##########
    0b00001111, 0b11110000,  //      ######
    0b00000000, 0b00000000,  //
    0b00111111, 0b11111100,  //   ############    (box opening)
    0b01111111, 0b11111110,  //  ##############
    0b01100000, 0b00000110,  //  ##          ##
    0b01100010, 0b01000110,  //  ##   #  #   ##   (items peeking)
    0b01100111, 0b11100110,  //  ##  ######  ##
    0b01100010, 0b01000110,  //  ##   #  #   ##
    0b01100000, 0b00000110,  //  ##          ##
    0b01111111, 0b11111110,  //  ##############
    0b01111111, 0b11111110,  //  ##############
    0b00111111, 0b11111100,  //   ############
    0b00001100, 0b00110000,  //     ##    ##
    0b00000000, 0b00000000   //
};

// Frame 3: Chest fully open - empty interior
const uint8_t TILE_CHEST_OPEN_16[32] PROGMEM = {
    0b01111111, 0b11111110,  //  ##############   (lid fully back)
    0b00111111, 0b11111100,  //   ############
    0b00000000, 0b00000000,  //
    0b00000000, 0b00000000,  //
    0b00111111, 0b11111100,  //   ############    (empty box)
    0b01111111, 0b11111110,  //  ##############
    0b01100000, 0b00000110,  //  ##          ##
    0b01100000, 0b00000110,  //  ##          ##
    0b01100000, 0b00000110,  //  ##          ##   (empty interior)
    0b01100000, 0b00000110,  //  ##          ##
    0b01100000, 0b00000110,  //  ##          ##
    0b01111111, 0b11111110,  //  ##############
    0b01111111, 0b11111110,  //  ##############
    0b00111111, 0b11111100,  //   ############
    0b00001100, 0b00110000,  //     ##    ##
    0b00000000, 0b00000000   //
};

// Log cabin sprite (16x16 for 2x2 building)
const uint8_t TILE_CABIN_16[32] PROGMEM = {
    0b00000110, 0b00000000,  //      ##            (chimney)
    0b00000110, 0b00000000,  //      ##
    0b00001111, 0b00000000,  //     ####           (roof peak)
    0b00111111, 0b11000000,  //   ########
    0b01111111, 0b11100000,  //  ##########
    0b11111111, 0b11110000,  // ############       (roof)
    0b01111111, 0b11100000,  //  ##########        (eaves)
    0b01100000, 0b00100000,  //  ##        #       (log walls)
    0b01111111, 0b11100000,  //  ##########        (log)
    0b01100110, 0b01100000,  //  ##  ##  ##        (windows)
    0b01101001, 0b01100000,  //  ## #  # ##        (window cross)
    0b01100110, 0b01100000,  //  ##  ##  ##
    0b01111111, 0b11100000,  //  ##########        (log)
    0b01100011, 0b00100000,  //  ##    ##  #       (door)
    0b01100010, 0b10100000,  //  ##    # # #       (door handle)
    0b11111111, 0b11110000   // ############       (foundation)
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

// Spirit NPC - ghostly figure
const uint8_t TILE_SPIRIT[8] PROGMEM = {
    0b00111100,  //   ####
    0b01111110,  //  ######
    0b01011010,  //  # ## #  (eyes)
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01010110,  //  # # ##
    0b01000010   //  #    #  (wavy bottom)
};

// Mountain - rocky peak
const uint8_t TILE_MOUNTAIN[8] PROGMEM = {
    0b00010000,  //    #
    0b00111000,  //   ###
    0b00111000,  //   ###
    0b01111100,  //  #####
    0b01111100,  //  #####
    0b11111110,  // #######
    0b11111110,  // #######
    0b11111111   // ########
};

// Snow ground - scattered dots
const uint8_t TILE_SNOW[8] PROGMEM = {
    0b10010010,  // #  #  #
    0b00100100,  //   #  #
    0b01001001,  //  #  #  #
    0b10010010,  // #  #  #
    0b00100100,  //   #  #
    0b01001001,  //  #  #  #
    0b10010010,  // #  #  #
    0b00100100   //   #  #
};

// Ice - frozen water surface
const uint8_t TILE_ICE[8] PROGMEM = {
    0b11111111,  // ########
    0b10000001,  // #      #
    0b10111101,  // # #### #
    0b10100101,  // # #  # #
    0b10100101,  // # #  # #
    0b10111101,  // # #### #
    0b10000001,  // #      #
    0b11111111   // ########
};

// Rabbit - small bunny creature
const uint8_t TILE_RABBIT[8] PROGMEM = {
    0b01000100,  //  #   #   (ears)
    0b01000100,  //  #   #
    0b00111000,  //   ###
    0b01111100,  //  #####
    0b01111100,  //  #####
    0b00111000,  //   ###
    0b00100100,  //   #  #
    0b00100100   //   #  #   (feet)
};

// Wolf - predator creature
const uint8_t TILE_WOLF[8] PROGMEM = {
    0b01000010,  //  #    #  (ears)
    0b11100111,  // ###  ###
    0b01111110,  //  ######
    0b00111100,  //   ####
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01000010,  //  #    #
    0b01000010   //  #    #  (legs)
};

// Snake - slithering creature
const uint8_t TILE_SNAKE[8] PROGMEM = {
    0b00011000,  //    ##    (head)
    0b00111100,  //   ####
    0b01100000,  //  ##
    0b00110000,  //   ##
    0b00011000,  //    ##
    0b00001100,  //     ##
    0b00000110,  //      ##
    0b00000011   //       ## (tail)
};

// Bear - large predator
const uint8_t TILE_BEAR[8] PROGMEM = {
    0b01100110,  //  ##  ##  (ears)
    0b11111111,  // ########
    0b11011011,  // ## ## ## (eyes)
    0b11111111,  // ########
    0b01111110,  //  ######
    0b01111110,  //  ######
    0b01100110,  //  ##  ##
    0b01100110   //  ##  ##  (legs)
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
#define TG_SPIRIT   'X'   // Spirit NPC (not stored in map, rendered separately)
#define TG_MOUNTAIN 'M'   // Mountain (extra energy, no building)
#define TG_SNOW     'n'   // Snow ground
#define TG_ICE      'i'   // Frozen water/lake

// Creature types (for getTileSprite, not stored in map)
#define TG_RABBIT   'R'
#define TG_WOLF     'W'
#define TG_SNAKE    'K'
#define TG_BEAR     'A'

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
        case TG_SPIRIT:   return TILE_SPIRIT;
        case TG_MOUNTAIN: return TILE_MOUNTAIN;
        case TG_SNOW:     return TILE_SNOW;
        case TG_ICE:      return TILE_ICE;
        // Creature sprites
        case TG_RABBIT:   return TILE_RABBIT;
        case TG_WOLF:     return TILE_WOLF;
        case TG_SNAKE:    return TILE_SNAKE;
        case TG_BEAR:     return TILE_BEAR;
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
