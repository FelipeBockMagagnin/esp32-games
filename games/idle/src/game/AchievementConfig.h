#ifndef ACHIEVEMENT_CONFIG_H
#define ACHIEVEMENT_CONFIG_H

#include <Arduino.h>
#include "GameConfig.h"
#include "CombatConfig.h"

// What an achievement watches. Each one is a threshold on a counter the game already
// tracks, so unlocking is a table scan rather than hand-written checks.
enum class AchKind : uint8_t
{
    TOTAL_CLICKS,     // Lifetime manual mines
    TOTAL_GOLD,       // Lifetime gold earned, whole units
    TOTAL_BUILDINGS,  // Sum of every building level
    BUILDING_LEVEL,   // Levels of BUILDINGS[target]
    UPGRADES_BOUGHT,  // Gold upgrades owned
    MINING_LEVEL,
    TOTAL_KILLS,      // Enemies killed across every zone
    ZONE_KILLS,       // Kills in ZONES[target]
    ITEMS_OWNED,      // Distinct items found
    ITEM_LEVEL,       // Highest level reached by any one item
    ACHIEVEMENTS      // Achievements already unlocked, so the list rewards itself
};

static const uint8_t NO_ACH_TARGET = 0xFF;

struct AchievementDef
{
    const char *name;        // Up to 13 chars to fit a list row title at text size 2
    const char *description; // Up to 28 chars: the row subtitle at size 1, beside a progress %
    AchKind kind;
    uint8_t target;       // Index into BUILDINGS or ZONES, else NO_ACH_TARGET
    uint64_t amount;      // Threshold the counter has to reach
    uint16_t bonusPercent; // Added to gold production once unlocked
};

static constexpr AchievementDef ACHIEVEMENTS[] = {
    //  name             description                          kind                        target                 amount        bonus%
    {"Handy Miner",  "Mine ore by hand 100 times",        AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         100,          1},
    {"Pickaxe Pro",  "Mine ore by hand 1000 times",       AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         1000,         3},
    {"Blistered",    "Mine ore by hand 10000 times",      AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         10000,        5},

    {"Pocket Money", "Earn 1000 gold in total",           AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000,         1},
    {"Gold Hoarder", "Earn 1M gold in total",             AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000000ULL,   2},
    {"Vault Keeper", "Earn 1B gold in total",             AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000000000ULL, 4},
    {"Trillionaire", "Earn 1T gold in total",             AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000000000000ULL, 6},

    {"Small Mine",   "Own 10 buildings",                  AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         10,           1},
    {"Mining Co.",   "Own 50 buildings",                  AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         50,           2},
    {"Industrialist", "Own 150 buildings",                AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         150,          4},
    {"Gold Tycoon",  "Own 300 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         300,          6},

    {"Pick Army",    "Own 25 Pickaxes",                   AchKind::BUILDING_LEVEL,   BUILDING_PICKAXE,      25,           2},
    {"Cart Fleet",   "Own 25 Minecarts",                  AchKind::BUILDING_LEVEL,   BUILDING_MINECART,     25,           2},
    {"Drill Team",   "Own 25 Drills",                     AchKind::BUILDING_LEVEL,   BUILDING_DRILL,        25,           3},
    {"Big Diggers",  "Own 25 Excavators",                 AchKind::BUILDING_LEVEL,   BUILDING_EXCAVATOR,    25,           3},
    {"Mill Works",   "Own 25 Ore Mills",                  AchKind::BUILDING_LEVEL,   BUILDING_ORE_MILL,     25,           4},
    {"Deep Industry", "Own 25 Deep Shafts",               AchKind::BUILDING_LEVEL,   BUILDING_DEEP_SHAFT,   25,           5},

    {"Shopper",      "Buy 5 upgrades",                    AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         5,            1},
    {"Collector",    "Buy 15 upgrades",                   AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         15,           3},
    {"Completionist", "Buy every upgrade",                AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         GOLD_UPGRADE_COUNT, 8},

    {"Apprentice",   "Reach mining level 10",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         10,           1},
    {"Journeyman",   "Reach mining level 25",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         25,           3},
    {"Master Miner", "Reach mining level 40",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         40,           5},

    {"Pest Control", "Defeat 10 enemies",                 AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         10,           1},
    {"Exterminator", "Defeat 100 enemies",                AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         100,          2},
    {"Slayer",       "Defeat 1000 enemies",               AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         1000,         4},
    {"Core Breaker", "Defeat 50 in the Molten Core",      AchKind::ZONE_KILLS,       ZONE_COUNT - 1,        50,           8},

    {"Geared Up",    "Find 9 different items",            AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         9,            1},
    {"Well Equipped", "Find 18 different items",          AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         18,           3},
    {"Hoarder",      "Find every item",                   AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         ITEM_COUNT,   8},

    {"Refined",      "Raise an item to level 10",         AchKind::ITEM_LEVEL,       NO_ACH_TARGET,         10,           2},
    {"Masterwork",   "Raise an item to level 25",         AchKind::ITEM_LEVEL,       NO_ACH_TARGET,         25,           4},
    {"Legendary",    "Raise an item to level 50",         AchKind::ITEM_LEVEL,       NO_ACH_TARGET,         50,           6},

    {"Decorated",    "Unlock 10 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         10,           3},
    {"Hall of Fame", "Unlock 25 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         25,           6},
};
static constexpr uint8_t ACHIEVEMENT_COUNT = sizeof(ACHIEVEMENTS) / sizeof(ACHIEVEMENTS[0]);

// How long a freshly unlocked achievement takes over the header of whatever screen is open
static const unsigned long ACHIEVEMENT_NOTICE_MS = 3500;

#endif // ACHIEVEMENT_CONFIG_H
