#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>

// Gold is stored in thousandths so fractional production (e.g. 0.1/s) stays exact
static const uint32_t GOLD_SCALE = 1000;
static const uint16_t GOLD_COLOR = 0xF685;

// Manual mining
static const uint32_t MINE_AMOUNT = 1 * GOLD_SCALE; // Gold gained per button click
static const uint32_t MINE_XP = 1;                 // Mining XP gained per click
static const uint32_t XP_PER_LEVEL = 10;           // XP needed for next level = XP_PER_LEVEL * level

// Buildings that generate gold automatically
struct BuildingsDef
{
    const char *name;
    uint32_t productionPerLevel;    // Per second, in GOLD_SCALE units (100 = 0.1/s)
    uint32_t baseCost;              // Whole gold units
    uint16_t costGrowthPercent;     // Cost multiplier per level bought (115 = +15%)
};

static constexpr BuildingsDef BUILDINGS[] = {
    //  name          gold/s  cost    growth%
    {"Pickaxe",       100,    10,     115},
    {"Minecart",      1000,   100,    115},
    {"Drill",         8000,   1100,   115},
    {"Excavator",     47000,  12000,  115},
};
// Derived from the table, so adding a row is all it takes to add a building
static constexpr uint8_t BUILDING_COUNT = sizeof(BUILDINGS) / sizeof(BUILDINGS[0]);

// One-time upgrades bought with gold that boost gold production
struct GoldUpgradeDef
{
    const char *name;         // Up to 14 chars to fit the detail box title
    const char *description;  // Wrapped over two lines of the detail box
    uint32_t cost;            // Whole gold units
    uint16_t bonusPercent;    // Added to gold production (100 = +100%); bonuses stack additively
};

static constexpr GoldUpgradeDef GOLD_UPGRADES[] = {
    //  name           description                                  cost    bonus%
    {"Sharp Picks",   "Gold production is increased by 30%",       10,     30},
    {"Oiled Carts",   "Gold production is increased by 50%",       50,     50},
    {"Gold Sense",    "Miners find twice as much gold, +100%",     250,    100},
    {"Deep Veins",    "Gold production is increased by 30%",       1000,   30},
    {"Refinery",      "Gold production is increased by 150%",      5000,   150},
    {"Midas Touch",   "Gold production is increased by 200%",      25000,  200},
};
static constexpr uint8_t GOLD_UPGRADE_COUNT = sizeof(GOLD_UPGRADES) / sizeof(GOLD_UPGRADES[0]);

#endif // GAME_CONFIG_H
