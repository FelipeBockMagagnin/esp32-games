#ifndef ROGUE_CONFIG_H
#define ROGUE_CONFIG_H

#include <stdint.h>

// What a map cell or entity looks like. The screen maps each to a sprite (TileMap.cpp), so
// the game logic never touches the art.
enum Glyph : uint8_t
{
    GLYPH_NONE,
    GLYPH_HERO,
    GLYPH_RAT,
    GLYPH_SLIME,
    GLYPH_TOAD,
    GLYPH_GOBLIN,
    GLYPH_ZOMBIE,
    GLYPH_WARLOCK,
    GLYPH_POTION,
    GLYPH_GOLD,
    GLYPH_DAGGER,
    GLYPH_SWORD,
    GLYPH_BROADSWORD,
    GLYPH_RUNEBLADE,
    GLYPH_LEATHER,
    GLYPH_CHAINMAIL,
    GLYPH_PLATE,
    GLYPH_COUNT
};

// The tiles fill the 240x288 under the HUD exactly, so the map never scrolls
static const uint8_t MAP_W = 15;
static const uint8_t MAP_H = 18;

static const int16_t PLAYER_BASE_HP = 20;
static const int16_t PLAYER_BASE_ATTACK = 3;
static const int16_t PLAYER_BASE_DEFENSE = 1;
static const int16_t HP_PER_LEVEL = 6;
static const uint8_t MAX_POTIONS = 9;
static const uint16_t REGEN_TURNS = 6; // One HP back every this many turns
static const uint8_t WAKE_DISTANCE = 5; // An enemy this close (in steps) starts chasing
static const uint16_t SCORE_PER_FLOOR = 100;
static const uint16_t SCORE_PER_KILL = 5;

// Total XP to reach level + 1: 10, 30, 60, 100...
inline uint16_t xpForLevel(uint8_t level)
{
    return 5u * level * (level + 1u);
}

// Floored at 1, unlike the idle game's: a turn-based fight against something the player
// cannot hurt would stall the run, and there is no idle grind to outlevel it
inline int16_t damageThrough(int16_t attack, int16_t defense)
{
    return attack - defense > 1 ? attack - defense : 1;
}

struct EnemyDef
{
    const char *name; // 9 chars max, for the log line
    Glyph glyph;
    int16_t hp;
    int16_t attack;
    int16_t defense;
    uint16_t xp;
    uint8_t minFloor;
};

// Each floor past an enemy's first adds 10% to its hp and attack (enemyStat). With these the
// greedy autoplay in tools/test.cpp dies on floor 7 (5 to 13), mostly to zombies and warlocks.
static constexpr EnemyDef ENEMIES[] = {
    {"Rat", GLYPH_RAT, 4, 2, 0, 2, 1},
    {"Slime", GLYPH_SLIME, 7, 2, 1, 3, 1},
    {"Toad", GLYPH_TOAD, 6, 4, 0, 5, 2},
    {"Goblin", GLYPH_GOBLIN, 9, 4, 1, 8, 3},
    {"Zombie", GLYPH_ZOMBIE, 14, 6, 2, 12, 5},
    {"Warlock", GLYPH_WARLOCK, 14, 9, 3, 20, 7},
};
static constexpr uint8_t ENEMY_COUNT = sizeof(ENEMIES) / sizeof(ENEMIES[0]);

inline int16_t enemyStat(int16_t base, uint8_t type, uint8_t floor)
{
    uint8_t past = floor > ENEMIES[type].minFloor ? floor - ENEMIES[type].minFloor : 0;
    return (int16_t)(base * (10 + past) / 10);
}

enum class ItemKind : uint8_t
{
    POTION,
    GOLD,
    WEAPON,
    ARMOR
};

struct ItemDef
{
    const char *name; // 10 chars max, for the log line
    Glyph glyph;
    ItemKind kind;
    int16_t value; // Attack or defense it adds; unused for potions and gold
    uint8_t minFloor;
};

// Must stay in table order
enum ItemId : uint8_t
{
    ITEM_POTION,
    ITEM_GOLD,
    ITEM_DAGGER,
    ITEM_SWORD,
    ITEM_BROADSWORD,
    ITEM_RUNEBLADE,
    ITEM_LEATHER,
    ITEM_CHAINMAIL,
    ITEM_PLATE,
};

static constexpr ItemDef ITEMS[] = {
    {"Potion", GLYPH_POTION, ItemKind::POTION, 0, 1},
    {"Gold", GLYPH_GOLD, ItemKind::GOLD, 0, 1},
    {"Dagger", GLYPH_DAGGER, ItemKind::WEAPON, 2, 1},
    {"Sword", GLYPH_SWORD, ItemKind::WEAPON, 4, 3},
    {"Broadsword", GLYPH_BROADSWORD, ItemKind::WEAPON, 7, 6},
    {"Runeblade", GLYPH_RUNEBLADE, ItemKind::WEAPON, 11, 9},
    {"Leather", GLYPH_LEATHER, ItemKind::ARMOR, 1, 1},
    {"Chainmail", GLYPH_CHAINMAIL, ItemKind::ARMOR, 3, 4},
    {"Plate", GLYPH_PLATE, ItemKind::ARMOR, 5, 7},
};
static constexpr uint8_t ITEM_COUNT = sizeof(ITEMS) / sizeof(ITEMS[0]);
static_assert(ITEMS[ITEM_PLATE].glyph == GLYPH_PLATE, "ItemId must follow the ITEMS order");

// Gear worse than what is worn turns into gold, this much per point of its value
static const uint16_t SCRAP_GOLD_PER_POINT = 5;

inline uint8_t enemiesOnFloor(uint8_t floor)
{
    uint8_t n = 2 + floor / 2;
    return n < 8 ? n : 8;
}

inline uint8_t itemsOnFloor(uint8_t floor)
{
    return floor < 3 ? 2 : 3;
}

#endif // ROGUE_CONFIG_H
