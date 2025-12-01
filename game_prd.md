# Touch Grass - Game Design Document

## Overview
Touch Grass is a tile-based survival exploration game for ESP32 with OLED display. Players explore a procedurally generated world, gather resources, and try to survive.

## Controls

| Button | World View | Menu/Inventory |
|--------|-----------|----------------|
| D-pad  | Move player | Navigate menu |
| A      | Interact with tile | Select action |
| B      | Open inventory | Go back |

## Tiles

| Tile | Letter | Description | Actions |
|------|--------|-------------|---------|
| Grass | `g` | Basic terrain | Cut (becomes Dirt) |
| Dirt | `d` | Cleared ground | Dig (yields Dirt item), Plant*, Build* |
| Water | `w` | Deep water impassable; shallow water swimmable | Place Dirt*, Examine |
| Tree | `t` | Walkable, can be cut | Cut* (requires Axe, yields Wood) |
| Chest | `x` | Contains starting items (one per map) | Open (yields 3 Seeds, Hammer, Axe) |
| Shrub | `s` | Harvestable plant | Harvest (yields Fruit, becomes Seedling) |
| Seedling | `p` | Growing plant | Examine (grows into Shrub after 35 actions) |
| Building | `b` | Player-built structure | Examine |

*Actions marked with * require specific items

## Items

| Item | Stackable | Use |
|------|-----------|-----|
| Seed | Yes (99) | Plant on Dirt to create Seedling |
| Fruit | Yes (99) | Eat to restore 25 hunger |
| Cooked Fruit | Yes (99) | Eat to restore 50 hunger (cook at stove) |
| Wood | Yes (99) | Used for building (3 required) |
| Dirt | Yes (99) | Place on Water to create land |
| Hammer | No | Required to build structures |
| Axe | No | Required to cut trees |

## Hunger System

- Starts at 250 (hidden, not displayed)
- Decreases by 1 for each:
  - Movement (any direction)
  - Action (cut, dig, plant, build, open, harvest, examine, eat)
- Eating Fruit restores 25 hunger (max 250)
- Reaching 0 hunger = **Death by starvation**

## Growth System

- Global action counter tracks all player actions
- After 35 actions, all Seedlings on the map become Shrubs
- Counter resets after growth occurs
- Shrubs can be harvested for Fruit (reverts to Seedling)

## Shallow Water (Swimming)

Water tiles can be shallow or deep based on adjacent water:
- **Shallow water**: Less than 2 adjacent water tiles - player can swim through
- **Deep water**: 2 or more adjacent water tiles - impassable

This allows players to cross isolated water tiles to reach blocked areas.

## Death Conditions

### Drowning
- Occurs when player has no escape route (no adjacent land or shallow water)
- Can happen after digging dirt into water while surrounded
- Adjacent = 4 cardinal directions (up, down, left, right)

### Starvation
- Occurs when hunger reaches 0
- Any action or movement when hunger is 1 will trigger death

## Map Generation

1. Fill map with Grass
2. Generate 1-2 rivers (random walk algorithm)
3. Generate 2-4 tree clusters
4. Scatter Dirt patches (~10% of grass)
5. Place one Chest on a Grass tile
6. Place player at center (or nearest valid position)

## Gameplay Loop

1. **Explore** - Find the Chest to get tools
2. **Gather** - Cut trees for Wood, harvest Shrubs for Fruit
3. **Sustain** - Plant Seeds, eat Fruit to survive
4. **Build** - Create structures with Hammer + 3 Wood
5. **Survive** - Avoid drowning and starvation

## Technical Notes

- Map size: 16x8 tiles (128x64 pixel display)
- Tile size: 8x8 pixels
- Player rendered as sprite, terrain as ASCII letters
- Inventory: 12 slots maximum
