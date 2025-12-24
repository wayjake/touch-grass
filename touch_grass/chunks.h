#ifndef TG_CHUNKS_H
#define TG_CHUNKS_H

#include <stdint.h>
#include "../shared/config.h"
#include "sprites.h"

// Chunk dimensions (same as screen)
#define CHUNK_WIDTH  MAP_WIDTH   // 16
#define CHUNK_HEIGHT MAP_HEIGHT  // 8

// Maximum cached chunks (memory constraint ~1.5KB)
#define MAX_CACHED_CHUNKS 5

// Biome types
enum BiomeType {
    BIOME_TEMPERATE = 0,
    BIOME_WOODSY,
    BIOME_ARID,
    BIOME_SNOW
};

// Chunk data structure
struct Chunk {
    int16_t x;                                    // Chunk X coordinate
    int16_t y;                                    // Chunk Y coordinate
    char terrain[CHUNK_HEIGHT][CHUNK_WIDTH];      // 128 bytes
    uint32_t seed;                                // Seed for regeneration
    bool generated;                               // Has terrain been generated?
    bool modified;                                // Has player modified this chunk?
    uint8_t lastAccessTime;                       // For LRU eviction (wrapping counter)
};

// Chunk cache
static Chunk chunkCache[MAX_CACHED_CHUNKS];
static uint8_t chunkCacheCount = 0;
static uint8_t accessCounter = 0;

// World seed (set at game start)
static uint32_t worldSeed = 12345;

// Player world position
static int16_t playerChunkX = 0;
static int16_t playerChunkY = 0;

// Camera position (which chunk is being displayed)
static int16_t cameraChunkX = 0;
static int16_t cameraChunkY = 0;

// Scrolling state
static int8_t scrollOffsetX = 0;  // Pixel offset during scroll animation
static bool isScrolling = false;

// Scroll threshold - start scrolling when player is this many tiles from edge
#define SCROLL_THRESHOLD 3

// Home chunk reference (from progression)
extern int16_t homeChunkX;
extern int16_t homeChunkY;

// Compute a deterministic seed for a chunk
static uint32_t computeChunkSeed(int16_t x, int16_t y) {
    // Hash function to combine chunk coords with world seed
    uint32_t hash = worldSeed;
    hash ^= (uint32_t)((x & 0xFFFF) << 16) | (y & 0xFFFF);
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

// Get biome for a chunk based on distance from home
static BiomeType getBiomeForChunk(int16_t chunkX, int16_t homeX) {
    // Distance from home in chunks (absolute X distance)
    int16_t distance = (chunkX >= homeX) ? (chunkX - homeX) : (homeX - chunkX);

    // Biome bands: every 3 chunks changes biome
    // 0: Temperate (home area)
    // 1-3: Woodsy/Mountains
    // 4-6: Arid
    // 7-9: Snow
    // 10+: Loop back
    int band = (distance / 3) % 4;

    switch (band) {
        case 0: return BIOME_TEMPERATE;
        case 1: return BIOME_WOODSY;
        case 2: return BIOME_ARID;
        case 3: return BIOME_SNOW;
        default: return BIOME_TEMPERATE;
    }
}

// Find a chunk in the cache, returns pointer or nullptr
static Chunk* findChunkInCache(int16_t x, int16_t y) {
    for (int i = 0; i < chunkCacheCount; i++) {
        if (chunkCache[i].x == x && chunkCache[i].y == y) {
            chunkCache[i].lastAccessTime = ++accessCounter;
            return &chunkCache[i];
        }
    }
    return nullptr;
}

// Find the least recently used chunk index for eviction
static int findLRUChunkIndex() {
    if (chunkCacheCount == 0) return -1;

    int lruIndex = 0;
    uint8_t oldestTime = chunkCache[0].lastAccessTime;

    for (int i = 1; i < chunkCacheCount; i++) {
        // Handle wrapping counter comparison
        int8_t diff = (int8_t)(chunkCache[i].lastAccessTime - oldestTime);
        if (diff < 0) {
            lruIndex = i;
            oldestTime = chunkCache[i].lastAccessTime;
        }
    }
    return lruIndex;
}

// Forward declarations for terrain generation
static void generateChunkTerrain(Chunk* chunk, BiomeType biome);

// Get or create a chunk (loads from cache or generates new)
static Chunk* getOrCreateChunk(int16_t x, int16_t y, int16_t homeX) {
    // Check if already in cache
    Chunk* existing = findChunkInCache(x, y);
    if (existing) {
        return existing;
    }

    // Need to create/load chunk
    int slotIndex;

    if (chunkCacheCount < MAX_CACHED_CHUNKS) {
        // Cache has room
        slotIndex = chunkCacheCount++;
    } else {
        // Evict LRU chunk
        slotIndex = findLRUChunkIndex();
    }

    // Initialize chunk
    Chunk* chunk = &chunkCache[slotIndex];
    chunk->x = x;
    chunk->y = y;
    chunk->seed = computeChunkSeed(x, y);
    chunk->generated = false;
    chunk->modified = false;
    chunk->lastAccessTime = ++accessCounter;

    // Generate terrain
    BiomeType biome = getBiomeForChunk(x, homeX);
    generateChunkTerrain(chunk, biome);

    return chunk;
}

// Initialize the chunk system
static void initChunkSystem(uint32_t seed) {
    worldSeed = seed;
    chunkCacheCount = 0;
    accessCounter = 0;
    playerChunkX = 0;
    playerChunkY = 0;
    cameraChunkX = 0;
    cameraChunkY = 0;
    scrollOffsetX = 0;
    isScrolling = false;
}

// Copy the initial map into chunk (0, 0)
static void setInitialChunk(char map[CHUNK_HEIGHT][CHUNK_WIDTH]) {
    Chunk* chunk = getOrCreateChunk(0, 0, 0);
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            chunk->terrain[y][x] = map[y][x];
        }
    }
    chunk->modified = true;  // Mark as modified so we don't regenerate
}

// Get the terrain tile at a world position
static char getWorldTile(int16_t chunkX, int16_t chunkY, int localX, int localY, int16_t homeX) {
    Chunk* chunk = getOrCreateChunk(chunkX, chunkY, homeX);
    if (chunk && localX >= 0 && localX < CHUNK_WIDTH && localY >= 0 && localY < CHUNK_HEIGHT) {
        return chunk->terrain[localY][localX];
    }
    return TG_WATER;  // Out of bounds = water
}

// Set a terrain tile at a world position
static void setWorldTile(int16_t chunkX, int16_t chunkY, int localX, int localY, char tile, int16_t homeX) {
    Chunk* chunk = getOrCreateChunk(chunkX, chunkY, homeX);
    if (chunk && localX >= 0 && localX < CHUNK_WIDTH && localY >= 0 && localY < CHUNK_HEIGHT) {
        chunk->terrain[localY][localX] = tile;
        chunk->modified = true;
    }
}

// Check if player should trigger horizontal scroll
static bool shouldScrollLeft(int localX) {
    return localX < SCROLL_THRESHOLD;
}

static bool shouldScrollRight(int localX) {
    return localX >= CHUNK_WIDTH - SCROLL_THRESHOLD;
}

// Generate terrain for a chunk based on biome
static void generateChunkTerrain(Chunk* chunk, BiomeType biome) {
    if (chunk->modified) {
        // Don't regenerate modified chunks
        chunk->generated = true;
        return;
    }

    // Seed random generator with chunk seed
    srand(chunk->seed);

    // Base tile based on biome
    char baseTile;
    switch (biome) {
        case BIOME_SNOW:
            baseTile = TG_SNOW;
            break;
        case BIOME_ARID:
            baseTile = TG_DIRT;
            break;
        case BIOME_TEMPERATE:
        case BIOME_WOODSY:
        default:
            baseTile = TG_GRASS;
            break;
    }

    // Fill with base tile
    for (int y = 0; y < CHUNK_HEIGHT; y++) {
        for (int x = 0; x < CHUNK_WIDTH; x++) {
            chunk->terrain[y][x] = baseTile;
        }
    }

    // Add biome-specific features
    switch (biome) {
        case BIOME_TEMPERATE:
            // Rivers, trees, shrubs (like original terrain)
            // Simple river (horizontal)
            if (rand() % 2 == 0) {
                int riverY = 1 + rand() % (CHUNK_HEIGHT - 2);
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    chunk->terrain[riverY][x] = TG_WATER;
                    if (rand() % 4 == 0 && riverY > 0) {
                        chunk->terrain[riverY - 1][x] = TG_WATER;
                    }
                }
            }
            // Tree clusters
            for (int i = 0; i < 2 + rand() % 3; i++) {
                int cx = rand() % CHUNK_WIDTH;
                int cy = rand() % CHUNK_HEIGHT;
                int radius = 1 + rand() % 2;
                for (int dy = -radius; dy <= radius; dy++) {
                    for (int dx = -radius; dx <= radius; dx++) {
                        int tx = cx + dx;
                        int ty = cy + dy;
                        if (tx >= 0 && tx < CHUNK_WIDTH && ty >= 0 && ty < CHUNK_HEIGHT) {
                            if (chunk->terrain[ty][tx] == TG_GRASS && rand() % 100 < 60) {
                                chunk->terrain[ty][tx] = TG_TREE;
                            }
                        }
                    }
                }
            }
            // Dirt patches
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    if (chunk->terrain[y][x] == TG_GRASS && rand() % 100 < 10) {
                        chunk->terrain[y][x] = TG_DIRT;
                    }
                }
            }
            break;

        case BIOME_WOODSY:
            // Dense trees with mountains
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    int r = rand() % 100;
                    if (r < 30) {
                        chunk->terrain[y][x] = TG_TREE;
                    } else if (r < 45) {
                        chunk->terrain[y][x] = TG_MOUNTAIN;
                    }
                }
            }
            break;

        case BIOME_ARID:
            // Sparse vegetation, occasional shrub
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    if (rand() % 100 < 5) {
                        chunk->terrain[y][x] = TG_SHRUB;
                    } else if (rand() % 100 < 3) {
                        chunk->terrain[y][x] = TG_TREE;
                    }
                }
            }
            break;

        case BIOME_SNOW:
            // Ice lakes, occasional trees
            // Ice lake
            if (rand() % 2 == 0) {
                int lakeX = 2 + rand() % (CHUNK_WIDTH - 4);
                int lakeY = 2 + rand() % (CHUNK_HEIGHT - 4);
                int lakeW = 2 + rand() % 4;
                int lakeH = 1 + rand() % 2;
                for (int dy = 0; dy < lakeH; dy++) {
                    for (int dx = 0; dx < lakeW; dx++) {
                        int tx = lakeX + dx;
                        int ty = lakeY + dy;
                        if (tx < CHUNK_WIDTH && ty < CHUNK_HEIGHT) {
                            chunk->terrain[ty][tx] = TG_ICE;
                        }
                    }
                }
            }
            // Sparse trees
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int x = 0; x < CHUNK_WIDTH; x++) {
                    if (chunk->terrain[y][x] == TG_SNOW && rand() % 100 < 8) {
                        chunk->terrain[y][x] = TG_TREE;
                    }
                }
            }
            break;
    }

    chunk->generated = true;
}

#endif
