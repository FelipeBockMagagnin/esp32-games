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
    ACHIEVEMENTS,     // Achievements already unlocked, so the list rewards itself
    VEINS_CAUGHT,     // Gold veins caught on the Mining screen
    GEAR_SET_MAXED,   // Gear pieces of tier `target` at ITEM_MAX_LEVEL, rings and amulets aside
    ITEM_MAXED        // Level of ITEMS[target]
};

// What an achievement pays, as `bonusPercent` of it. Gold production is the default; the
// gear sets pay combat stats and the rings and amulets a utility each. Cuts take a
// percentage off a wait, the rest add a percentage.
enum class AchReward : uint8_t
{
    PRODUCTION,
    ATTACK_DEFENSE,     // Both, added to the upgrades' attack and defense bonuses
    RESPAWN_CUT,        // Shares MAX_RESPAWN_CUT_PERCENT with the items' cut
    REVIVE_CUT,
    GUARD_TIME,
    SPELL_COOLDOWN_CUT,
    KILL_GOLD,
    DROP_CHANCE,        // Scales every drop's chance, not added to it
    COUNT
};
static constexpr uint8_t ACH_REWARD_COUNT = (uint8_t)AchReward::COUNT;

// Rings, amulets and relics are jewellery; the rest is gear, which a set needs maxed
static constexpr bool isGearSlot(EquipSlot slot)
{
    return slot != EquipSlot::RING && slot != EquipSlot::AMULET && slot != EquipSlot::RELIC;
}

static constexpr uint8_t gearInTier(uint8_t tier, uint8_t i = 0)
{
    return i >= ITEM_COUNT ? 0 : (ITEMS[i].tier == tier && isGearSlot(ITEMS[i].slot)) + gearInTier(tier, i + 1);
}

static const uint8_t NO_ACH_TARGET = 0xFF;

struct AchievementDef
{
    const char *name;        // Up to 13 chars to fit a list row title at text size 2
    const char *description; // Up to 28 chars: the row subtitle at size 1, beside a progress %
    AchKind kind;
    uint8_t target;       // Index into BUILDINGS, ZONES or ITEMS, or an item tier; else NO_ACH_TARGET
    uint64_t amount;      // Threshold the counter has to reach
    uint16_t bonusPercent; // Size of the reward once unlocked
    AchReward reward;     // Left off the production rows, so it zero-initializes to PRODUCTION
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
    {"Big Spender",  "Buy 35 upgrades",                   AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         35,           5},
    {"Completionist", "Buy 60 upgrades",                  AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         60,           8},

    {"Apprentice",   "Reach mining level 10",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         10,           1},
    {"Journeyman",   "Reach mining level 25",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         25,           3},
    {"Master Miner", "Reach mining level 40",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         40,           5},

    {"Pest Control", "Defeat 10 enemies",                 AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         10,           1},
    {"Exterminator", "Defeat 100 enemies",                AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         100,          2},
    {"Slayer",       "Defeat 1000 enemies",               AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         1000,         4},
    {"Core Breaker", "Defeat 50 in the Molten Core",      AchKind::ZONE_KILLS,       ZONE_MOLTEN_CORE,      50,           8},
    {"Star Breaker", "Defeat 50 in the Star Forge",       AchKind::ZONE_KILLS,       ZONE_COUNT - 1,        50,           10},

    {"Geared Up",    "Find 9 different items",            AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         9,            1},
    {"Well Equipped", "Find 18 different items",          AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         18,           3},
    {"Hoarder",      "Find every item",                   AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         ITEM_COUNT,   8},

    {"Refined",      "Raise an item to level 4",          AchKind::ITEM_LEVEL,       NO_ACH_TARGET,         4,           2},
    {"Masterwork",   "Raise an item to level 7",          AchKind::ITEM_LEVEL,       NO_ACH_TARGET,         7,           4},
    {"Legendary",    "Raise an item to max level",        AchKind::ITEM_LEVEL,       NO_ACH_TARGET,         ITEM_MAX_LEVEL,           6},

    {"Lucky Find",   "Catch a gold vein",                 AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         1,            1},
    {"Prospector",   "Catch 25 gold veins",               AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         25,           3},
    {"Vein Hunter",  "Catch 100 gold veins",              AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         100,          6},

    // A maxed set lands well after its zone, so its stats help the zones ahead rather
    // than the one that dropped it
    {"Scrap Knight", "Max all Common gear",               AchKind::GEAR_SET_MAXED,   0,                     gearInTier(0), 10, AchReward::ATTACK_DEFENSE},
    {"Iron Will",    "Max all Uncommon gear",             AchKind::GEAR_SET_MAXED,   1,                     gearInTier(1), 15, AchReward::ATTACK_DEFENSE},
    {"Steel Wall",   "Max all Rare gear",                 AchKind::GEAR_SET_MAXED,   2,                     gearInTier(2), 20, AchReward::ATTACK_DEFENSE},
    {"Rune Lord",    "Max all Epic gear",                 AchKind::GEAR_SET_MAXED,   3,                     gearInTier(3), 25, AchReward::ATTACK_DEFENSE},
    {"Mythbreaker",  "Max all Mythic gear",               AchKind::GEAR_SET_MAXED,   MYTHIC_TIER,           gearInTier(MYTHIC_TIER), 30, AchReward::ATTACK_DEFENSE},

    {"Tin Polish",   "Max the Tin Ring",                  AchKind::ITEM_MAXED,       ITEM_TIN_RING,         ITEM_MAX_LEVEL, 25, AchReward::REVIVE_CUT},
    {"Gilded Guard", "Max the Gold Ring",                 AchKind::ITEM_MAXED,       ITEM_GOLD_RING,        ITEM_MAX_LEVEL, 33, AchReward::GUARD_TIME},
    {"Ruby Luck",    "Max the Ruby Ring",                 AchKind::ITEM_MAXED,       ITEM_RUBY_RING,        ITEM_MAX_LEVEL, 10, AchReward::DROP_CHANCE},
    {"Miner Pride",  "Max the Miner Ring",                AchKind::ITEM_MAXED,       ITEM_MINER_RING,       ITEM_MAX_LEVEL, 50, AchReward::KILL_GOLD},
    {"Best Friend",  "Max Wilson",                        AchKind::ITEM_MAXED,       ITEM_WILSON,           ITEM_MAX_LEVEL, 15, AchReward::RESPAWN_CUT},
    {"Jade Focus",   "Max the Jade Amulet",               AchKind::ITEM_MAXED,       ITEM_JADE_AMULET,      ITEM_MAX_LEVEL, 15, AchReward::SPELL_COOLDOWN_CUT},
    {"Golden Hour",  "Max the Gold Amulet",               AchKind::ITEM_MAXED,       ITEM_GOLD_AMULET,      ITEM_MAX_LEVEL, 25, AchReward::KILL_GOLD},
    {"Core Echo",    "Max the Core Sigil",                AchKind::ITEM_MAXED,       ITEM_CORE_SIGIL,       ITEM_MAX_LEVEL, 20, AchReward::SPELL_COOLDOWN_CUT},

    {"Decorated",    "Unlock 10 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         10,           3},
    {"Hall of Fame", "Unlock 25 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         25,           6},
};
static constexpr uint8_t ACHIEVEMENT_COUNT = sizeof(ACHIEVEMENTS) / sizeof(ACHIEVEMENTS[0]);

// How long a freshly unlocked achievement takes over the header of whatever screen is open
static const unsigned long ACHIEVEMENT_NOTICE_MS = 3500;

#endif // ACHIEVEMENT_CONFIG_H
