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
    ACHIEVEMENTS,     // Achievements already unlocked, so the list rewards itself
    VEINS_CAUGHT,     // Gold veins caught on the Mining screen
    GEAR_SET_MAXED,   // Gear pieces of tier `target` at ITEM_MAX_LEVEL, jewellery and tools aside
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

// Rings, amulets and relics are jewellery; the rest is gear, and the gear that is not a
// tool makes up a tier's set
static constexpr bool isSetPiece(const ItemDef &item)
{
    return !item.tool && item.slot != EquipSlot::RING && item.slot != EquipSlot::AMULET && item.slot != EquipSlot::RELIC;
}

static constexpr uint8_t gearInTier(uint8_t tier, uint8_t i = 0)
{
    return i >= ITEM_COUNT ? 0 : (ITEMS[i].tier == tier && isSetPiece(ITEMS[i])) + gearInTier(tier, i + 1);
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

// Clicks, veins, kills, levels and run gold restart with each ascension, so their top tiers
// are sized for a run of about six days; the "Own 50" rows and the item ones are the goals
// that take several runs (tools/sim --days 6 --runs 15 unlocks the last around day 66).
static constexpr AchievementDef ACHIEVEMENTS[] = {
    //  name             description                          kind                        target                 amount        bonus%
    {"Handy Miner",  "Mine ore by hand 100 times",        AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         100,          1},
    {"Pickaxe Pro",  "Mine ore by hand 1000 times",       AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         1000,         3},
    {"Blistered",    "Mine ore by hand 10000 times",      AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         10000,        5},
    {"Carpal Tunnel", "Mine ore by hand 40000 times",      AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         40000,        3},
    {"Golden Hand",   "Mine ore by hand 80K times",        AchKind::TOTAL_CLICKS,     NO_ACH_TARGET,         80000,        4},

    {"Pocket Money", "Earn 1000 gold in total",           AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000,         1},
    {"Gold Hoarder", "Earn 1M gold in total",             AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000000ULL,   2},
    {"Vault Keeper", "Earn 1B gold in total",             AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000000000ULL, 4},
    {"Trillionaire", "Earn 1T gold in total",             AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000000000000ULL, 6},
    {"Midas Heir",    "Earn 100T gold in total",           AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         100000000000000ULL, 4},
    {"Gold Mountain", "Earn 1Q gold in total",             AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         1000000000000000ULL, 5},
    {"Dragon Hoard",  "Earn 10Q gold in total",            AchKind::TOTAL_GOLD,       NO_ACH_TARGET,         10000000000000000ULL, 6},

    {"First Claim",   "Own 1 building",                    AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         1,            1},
    {"Small Mine",    "Own 10 buildings",                  AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         10,           1},
    {"Hamlet",        "Own 25 buildings",                  AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         25,           1},
    {"Mining Co.",    "Own 50 buildings",                  AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         50,           2},
    {"Industrialist", "Own 100 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         100,          3},
    {"Company Town",  "Own 200 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         200,          4},
    {"Gold Tycoon",   "Own 300 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         300,          5},
    {"Magnate",       "Own 400 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         400,          5},
    {"Boom Town",     "Own 500 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         500,          6},
    {"Metropolis",    "Own 600 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         600,          6},
    {"Megacorp",      "Own 700 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         700,          7},
    {"Mining Empire", "Own 800 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         800,          7},
    {"Deep Dominion", "Own 900 buildings",                 AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         900,          8},
    {"Earth Eater",   "Own 1000 buildings",                AchKind::TOTAL_BUILDINGS,  NO_ACH_TARGET,         1000,         10},

    {"Pick Army",    "Own 25 Pickaxes",                   AchKind::BUILDING_LEVEL,   BUILDING_PICKAXE,      25,           2},
    {"Cart Fleet",   "Own 25 Minecarts",                  AchKind::BUILDING_LEVEL,   BUILDING_MINECART,     25,           2},
    {"Drill Team",   "Own 25 Drills",                     AchKind::BUILDING_LEVEL,   BUILDING_DRILL,        25,           3},
    {"Big Diggers",  "Own 25 Excavators",                 AchKind::BUILDING_LEVEL,   BUILDING_EXCAVATOR,    25,           3},
    {"Mill Works",   "Own 25 Ore Mills",                  AchKind::BUILDING_LEVEL,   BUILDING_ORE_MILL,     25,           4},
    {"Smelt Shop",    "Own 25 Smelters",                   AchKind::BUILDING_LEVEL,   BUILDING_SMELTER,      25,           2},
    {"Deep Industry", "Own 25 Deep Shafts",               AchKind::BUILDING_LEVEL,   BUILDING_DEEP_SHAFT,   25,           5},
    {"Rune Guild",    "Own 25 Rune Forges",                AchKind::BUILDING_LEVEL,   BUILDING_RUNE_FORGE,   25,           3},
    {"Barge Fleet",   "Own 25 Ore Barges",                 AchKind::BUILDING_LEVEL,   BUILDING_ORE_BARGE,    25,           3},
    {"Alchemist",     "Own 25 Transmuters",                AchKind::BUILDING_LEVEL,   BUILDING_TRANSMUTER,   25,           3},
    {"Void Gazer",    "Own 25 Void Pits",                  AchKind::BUILDING_LEVEL,   BUILDING_VOID_PIT,     25,           3},
    {"Timekeeper",    "Own 25 Time Mines",                 AchKind::BUILDING_LEVEL,   BUILDING_TIME_MINE,    25,           4},
    {"Stargazer",     "Own 25 Star Mines",                 AchKind::BUILDING_LEVEL,   BUILDING_STAR_MINE,    25,           4},
    {"Pick Legion",   "Own 50 Pickaxes",                   AchKind::BUILDING_LEVEL,   BUILDING_PICKAXE,      50,           1},
    {"Cart Empire",   "Own 50 Minecarts",                  AchKind::BUILDING_LEVEL,   BUILDING_MINECART,     50,           1},
    {"Drill Corps",   "Own 50 Drills",                     AchKind::BUILDING_LEVEL,   BUILDING_DRILL,        50,           2},
    {"Earthmovers",   "Own 50 Excavators",                 AchKind::BUILDING_LEVEL,   BUILDING_EXCAVATOR,    50,           2},
    {"Mill Town",     "Own 50 Ore Mills",                  AchKind::BUILDING_LEVEL,   BUILDING_ORE_MILL,     50,           2},
    {"Foundry",       "Own 50 Smelters",                   AchKind::BUILDING_LEVEL,   BUILDING_SMELTER,      50,           2},
    {"Hollow Earth",  "Own 50 Deep Shafts",                AchKind::BUILDING_LEVEL,   BUILDING_DEEP_SHAFT,   50,           2},
    {"Runesmith",     "Own 50 Rune Forges",                AchKind::BUILDING_LEVEL,   BUILDING_RUNE_FORGE,   50,           2},
    {"Armada",        "Own 50 Ore Barges",                 AchKind::BUILDING_LEVEL,   BUILDING_ORE_BARGE,    50,           3},
    {"Philosopher",   "Own 50 Transmuters",                AchKind::BUILDING_LEVEL,   BUILDING_TRANSMUTER,   50,           3},
    {"Void Walker",   "Own 50 Void Pits",                  AchKind::BUILDING_LEVEL,   BUILDING_VOID_PIT,     50,           3},
    {"Epoch Master",  "Own 50 Time Mines",                 AchKind::BUILDING_LEVEL,   BUILDING_TIME_MINE,    50,           3},
    {"Constellation", "Own 50 Star Mines",                 AchKind::BUILDING_LEVEL,   BUILDING_STAR_MINE,    50,           4},

    {"Shopper",      "Buy 5 upgrades",                    AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         5,            1},
    {"Collector",    "Buy 15 upgrades",                   AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         15,           3},
    {"Big Spender",  "Buy 35 upgrades",                   AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         35,           5},
    {"Completionist", "Buy 60 upgrades",                  AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         60,           8},
    {"Connoisseur",   "Buy 80 upgrades",                   AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         80,           5},
    {"Shopaholic",    "Buy 90 upgrades",                   AchKind::UPGRADES_BOUGHT,  NO_ACH_TARGET,         90,           6},

    {"Apprentice",   "Reach mining level 10",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         10,           1},
    {"Journeyman",   "Reach mining level 25",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         25,           3},
    {"Master Miner", "Reach mining level 40",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         40,           5},
    {"Expert Miner",  "Reach mining level 75",             AchKind::MINING_LEVEL,     NO_ACH_TARGET,         75,           3},
    {"Grandmaster",   "Reach mining level 120",            AchKind::MINING_LEVEL,     NO_ACH_TARGET,         120,          4},
    {"Living Legend", "Reach mining level 175",            AchKind::MINING_LEVEL,     NO_ACH_TARGET,         175,          5},

    {"Pest Control", "Defeat 10 enemies",                 AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         10,           1},
    {"Exterminator", "Defeat 100 enemies",                AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         100,          2},
    {"Slayer",       "Defeat 1000 enemies",               AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         1000,         4},
    {"Warlord",       "Defeat 10000 enemies",              AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         10000,        3},
    {"Annihilator",   "Defeat 50000 enemies",              AchKind::TOTAL_KILLS,      NO_ACH_TARGET,         50000,        4},
    {"Core Breaker", "Defeat 50 in the Molten Core",      AchKind::ZONE_KILLS,       ZONE_MOLTEN_CORE,      50,           8},
    {"Star Breaker", "Defeat 50 in the Star Forge",       AchKind::ZONE_KILLS,       ZONE_COUNT - 1,        50,           10},

    {"Geared Up",    "Find 9 different items",            AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         9,            1},
    {"Well Equipped", "Find 18 different items",          AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         18,           3},
    {"Curator",      "Find 40 different items",           AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         40,           10, AchReward::DROP_CHANCE},
    {"Hoarder",      "Find every item",                   AchKind::ITEMS_OWNED,      NO_ACH_TARGET,         ITEM_COUNT,   8},

    {"Lucky Find",   "Catch a gold vein",                 AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         1,            1},
    {"Prospector",   "Catch 25 gold veins",               AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         25,           3},
    {"Vein Hunter",  "Catch 100 gold veins",              AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         100,          6},
    {"Vein Master",   "Catch 200 gold veins",              AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         200,          4},
    {"Lode Seeker",   "Catch 300 gold veins",              AchKind::VEINS_CAUGHT,     NO_ACH_TARGET,         300,          5},

    // An item's level only pays an award for a whole set, a pick, an amulet or a ring, maxed.
    // A maxed set lands well after its zone, so its stats help the zones ahead rather than
    // the one that dropped it; with the levels carried across ascensions, these are the
    // long goals of the game.
    {"Scrap Knight", "Max all Common gear",               AchKind::GEAR_SET_MAXED,   0,                     gearInTier(0), 10, AchReward::ATTACK_DEFENSE},
    {"Iron Will",    "Max all Uncommon gear",             AchKind::GEAR_SET_MAXED,   1,                     gearInTier(1), 15, AchReward::ATTACK_DEFENSE},
    {"Steel Wall",   "Max all Rare gear",                 AchKind::GEAR_SET_MAXED,   2,                     gearInTier(2), 20, AchReward::ATTACK_DEFENSE},
    {"Rune Lord",    "Max all Epic gear",                 AchKind::GEAR_SET_MAXED,   3,                     gearInTier(3), 25, AchReward::ATTACK_DEFENSE},
    {"Mythbreaker",  "Max all Mythic gear",               AchKind::GEAR_SET_MAXED,   MYTHIC_TIER,           gearInTier(MYTHIC_TIER), 30, AchReward::ATTACK_DEFENSE},

    // Picks pay gold production
    {"Rust Bucket",  "Max the Rusty Pick",                AchKind::ITEM_MAXED,       ITEM_RUSTY_PICK,       ITEM_MAX_LEVEL, 5},
    {"Iron Digger",  "Max the Iron Pick",                 AchKind::ITEM_MAXED,       ITEM_IRON_PICK,        ITEM_MAX_LEVEL, 10},
    {"Steel Digger", "Max the Steel Pick",                AchKind::ITEM_MAXED,       ITEM_STEEL_PICK,       ITEM_MAX_LEVEL, 15},
    {"Rune Digger",  "Max the Rune Pick",                 AchKind::ITEM_MAXED,       ITEM_RUNE_PICK,        ITEM_MAX_LEVEL, 20},
    {"Void Digger",  "Max the Void Pick",                 AchKind::ITEM_MAXED,       ITEM_VOID_PICK,        ITEM_MAX_LEVEL, 30},

    // Amulets pay a utility each
    {"Best Friend",  "Max Wilson",                        AchKind::ITEM_MAXED,       ITEM_WILSON,           ITEM_MAX_LEVEL, 15, AchReward::RESPAWN_CUT},
    {"Jade Focus",   "Max the Jade Amulet",               AchKind::ITEM_MAXED,       ITEM_JADE_AMULET,      ITEM_MAX_LEVEL, 15, AchReward::SPELL_COOLDOWN_CUT},
    {"Golden Hour",  "Max the Gold Amulet",               AchKind::ITEM_MAXED,       ITEM_GOLD_AMULET,      ITEM_MAX_LEVEL, 25, AchReward::KILL_GOLD},
    {"Core Echo",    "Max the Core Sigil",                AchKind::ITEM_MAXED,       ITEM_CORE_SIGIL,       ITEM_MAX_LEVEL, 15, AchReward::DROP_CHANCE},
    {"Ember Soul",   "Max the Ember Charm",               AchKind::ITEM_MAXED,       ITEM_EMBER_CHARM,      ITEM_MAX_LEVEL, 25, AchReward::REVIVE_CUT},

    // Rings alternate between the two
    {"Tin Polish",   "Max the Tin Ring",                  AchKind::ITEM_MAXED,       ITEM_TIN_RING,         ITEM_MAX_LEVEL, 5},
    {"Golden Luck",  "Max the Gold Ring",                 AchKind::ITEM_MAXED,       ITEM_GOLD_RING,        ITEM_MAX_LEVEL, 10, AchReward::DROP_CHANCE},
    {"Ruby Guard",   "Max the Ruby Ring",                 AchKind::ITEM_MAXED,       ITEM_RUBY_RING,        ITEM_MAX_LEVEL, 33, AchReward::GUARD_TIME},
    {"Coin Hoard",   "Max the Coin Ring",                 AchKind::ITEM_MAXED,       ITEM_COIN_RING,        ITEM_MAX_LEVEL, 15},
    {"Miner Pride",  "Max the Miner Ring",                AchKind::ITEM_MAXED,       ITEM_MINER_RING,       ITEM_MAX_LEVEL, 20},
    {"Ember Pride",  "Max the Ember Ring",                AchKind::ITEM_MAXED,       ITEM_EMBER_RING,       ITEM_MAX_LEVEL, 50, AchReward::KILL_GOLD},

    {"Decorated",    "Unlock 10 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         10,           3},
    {"Hall of Fame", "Unlock 25 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         25,           6},
    {"Laureate",      "Unlock 50 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         50,           4},
    {"Immortal",      "Unlock 75 achievements",            AchKind::ACHIEVEMENTS,     NO_ACH_TARGET,         75,           5},
};
static constexpr uint8_t ACHIEVEMENT_COUNT = sizeof(ACHIEVEMENTS) / sizeof(ACHIEVEMENTS[0]);

// How long a freshly unlocked achievement takes over the header of whatever screen is open
static const unsigned long ACHIEVEMENT_NOTICE_MS = 3500;

#endif // ACHIEVEMENT_CONFIG_H
