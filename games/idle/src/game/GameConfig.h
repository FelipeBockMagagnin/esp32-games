#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>

// Gold is stored in thousandths so fractional production (e.g. 0.1/s) stays exact
static const uint32_t GOLD_SCALE = 1000;
static const uint16_t GOLD_COLOR = 0xF685;

// Manual mining
static const uint32_t MINE_XP = 1;         // Mining XP gained per click
static const uint32_t XP_PER_LEVEL = 10;   // XP needed for next level = XP_PER_LEVEL * level

// The ore mined changes every LEVELS_PER_TIER levels, both its name and its base click gold
struct OreTierDef
{
    const char *name;
    uint32_t clickAmount; // Whole gold units gained per click at this tier, before click upgrades
};

static constexpr OreTierDef ORE_TIERS[] = {
    //  name             gold/click
    {"Copper Ore",       1},
    {"Tin Ore",          2},
    {"Iron Ore",         4},
    {"Silver Ore",       8},
    {"Gold Ore",         16},
    {"Platinum Ore",     32},
    {"Diamond Ore",      64},
    {"Mythril Ore",      128},
};
static constexpr uint8_t ORE_TIER_COUNT = sizeof(ORE_TIERS) / sizeof(ORE_TIERS[0]);
static constexpr uint8_t LEVELS_PER_TIER = 5; // Mining levels spent on each ore before the next

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

// What a one-time gold upgrade boosts: automatic production, or the manual mine click
enum class UpgradeTarget : uint8_t
{
    PRODUCTION,
    CLICK
};

// One-time upgrades bought with gold that boost gold gain
struct GoldUpgradeDef
{
    const char *name;         // Up to 14 chars to fit the detail box title
    const char *description;  // Wrapped over two lines of the detail box
    uint32_t cost;            // Whole gold units
    uint16_t bonusPercent;    // Added to the target (100 = +100%); bonuses of the same target stack additively
    UpgradeTarget target;
};

static constexpr GoldUpgradeDef GOLD_UPGRADES[] = {
    //  name           description                                  cost    bonus%  target
    {"Sharp Picks",   "Gold production is increased by 30%",       10,     30,     UpgradeTarget::PRODUCTION},
    {"Oiled Carts",   "Gold production is increased by 50%",       50,     50,     UpgradeTarget::PRODUCTION},
    {"Gold Sense",    "Miners find twice as much gold, +100%",     250,    100,    UpgradeTarget::PRODUCTION},
    {"Deep Veins",    "Gold production is increased by 30%",       1000,   30,     UpgradeTarget::PRODUCTION},
    {"Refinery",      "Gold production is increased by 150%",      5000,   150,    UpgradeTarget::PRODUCTION},
    {"Midas Touch",   "Gold production is increased by 200%",      25000,  200,    UpgradeTarget::PRODUCTION},
    {"Iron Fist",     "Gold per click is increased by 50%",        15,     50,     UpgradeTarget::CLICK},
    {"Steel Grip",    "Gold per click is increased by 50%",        75,     50,     UpgradeTarget::CLICK},
    {"Power Swing",   "Gold per click is increased by 100%",       375,    100,    UpgradeTarget::CLICK},
    {"Golden Touch",  "Gold per click is increased by 100%",       1500,   100,    UpgradeTarget::CLICK},
    {"Crit Strike",   "Gold per click is increased by 150%",       7500,   150,    UpgradeTarget::CLICK},
    {"Ore Sense",     "Gold per click is increased by 200%",       30000,  200,    UpgradeTarget::CLICK},
};
static constexpr uint8_t GOLD_UPGRADE_COUNT = sizeof(GOLD_UPGRADES) / sizeof(GOLD_UPGRADES[0]);

#endif // GAME_CONFIG_H
