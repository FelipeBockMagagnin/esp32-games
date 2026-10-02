#ifndef COMBAT_CONFIG_H
#define COMBAT_CONFIG_H

#include <Arduino.h>
#include "ItemConfig.h"
#include "../assets/Assets.h"

// Combat pacing. The auto-attack runs on its own clock so a fight progresses on any
// screen; the two manual actions are what the player adds by being present.
static const unsigned long AUTO_ATTACK_MS = 2000;
static const unsigned long STRIKE_COOLDOWN_MS = 3000;
static const uint8_t STRIKE_MULTIPLIER = 2; // Manual strike hits this many times as hard
// Smite is the heavy hit an equipped amulet grants. The multiplier is what lets it reach
// defenses the ordinary strike cannot, and the long cooldown is what keeps it from
// out-damaging the strike over time.
static const uint8_t SMITE_MULTIPLIER = 4;
static const unsigned long SMITE_COOLDOWN_MS = 12000;

static const unsigned long GUARD_COOLDOWN_MS = 9000;
static const unsigned long GUARD_DURATION_MS = 3000;
static const uint8_t GUARD_DAMAGE_PERCENT = 50; // Incoming damage while guarding
static const unsigned long RESPAWN_MS = 1500;
static const unsigned long REVIVE_MS = 8000; // Health refills over this long after a death

// Chance is in basis points, so 10000 is a guaranteed drop
struct DropDef
{
    uint8_t itemId;
    uint16_t chanceBp;
};

struct EnemyDef
{
    const char *name;
    uint32_t maxHp;
    uint32_t attack;
    uint32_t defense;
    unsigned long attackIntervalMs;
    uint32_t goldReward;
    // Every enemy shares the one converted sprite for now; swapping art is a pointer change
    const uint16_t *sprite;
    int16_t spriteW;
    int16_t spriteH;
    const DropDef *drops;
    uint8_t dropCount;
};

struct ZoneDef
{
    const char *name; // Up to 11 chars to fit a list row title at text size 2
    const EnemyDef *enemies;
    uint8_t enemyCount;
    bool requiresUnlockUpgrade; // Needs the UNLOCK_ZONE upgrade that carries this zone index
    uint16_t reqPrevZoneKills;  // Kills needed in the zone before it; 0 for none
    const uint16_t *icon;       // 16x16 RGB565, black transparent
};

#define RAT_SPRITE image_rato_pixels, 128, 128

static constexpr DropDef SEWER_RAT_DROPS[] = {
    {ITEM_RUSTY_SWORD, 550},
    {ITEM_CLOTH_HOOD, 550},
    {ITEM_RAG_GLOVES, 550},
};
static constexpr DropDef SEWER_SLIME_DROPS[] = {
    {ITEM_CLOTH_VEST, 550},
    {ITEM_CLOTH_PANTS, 550},
    {ITEM_WORN_BOOTS, 550},
    {ITEM_WOOD_SHIELD, 400},
    {ITEM_TIN_RING, 200},
    {ITEM_BONE_CHARM, 200},
};

static constexpr DropDef CAVE_BAT_DROPS[] = {
    {ITEM_IRON_HELM, 400},
    {ITEM_IRON_GLOVES, 400},
    {ITEM_IRON_SWORD, 400},
};
static constexpr DropDef ROCK_CRAWLER_DROPS[] = {
    {ITEM_IRON_MAIL, 400},
    {ITEM_IRON_LEGS, 400},
    {ITEM_IRON_BOOTS, 400},
    {ITEM_IRON_SHIELD, 300},
    {ITEM_GOLD_RING, 100},
    {ITEM_JADE_AMULET, 100},
};

static constexpr DropDef BONE_DIGGER_DROPS[] = {
    {ITEM_STEEL_HELM, 400},
    {ITEM_STEEL_GRIPS, 400},
    {ITEM_STEEL_SWORD, 400},
};
static constexpr DropDef CRYPT_GHOUL_DROPS[] = {
    {ITEM_STEEL_PLATE, 400},
    {ITEM_STEEL_LEGS, 400},
    {ITEM_STEEL_BOOTS, 400},
    {ITEM_STEEL_GUARD, 300},
    {ITEM_RUBY_RING, 100},
    {ITEM_GOLD_AMULET, 100},
};

static constexpr DropDef DEEP_LURKER_DROPS[] = {
    {ITEM_RUNE_HELM, 400},
    {ITEM_RUNE_GRIPS, 400},
    {ITEM_RUNE_BLADE, 400},
};
static constexpr DropDef ABYSS_WORM_DROPS[] = {
    {ITEM_RUNE_PLATE, 400},
    {ITEM_RUNE_LEGS, 400},
    {ITEM_RUNE_BOOTS, 400},
    {ITEM_RUNE_AEGIS, 300},
};

static constexpr DropDef MAGMA_HOUND_DROPS[] = {
    {ITEM_RUNE_BLADE, 400},
    {ITEM_RUNE_AEGIS, 300},
    {ITEM_MINER_RING, 100},
};
static constexpr DropDef CORE_GOLEM_DROPS[] = {
    {ITEM_RUNE_PLATE, 400},
    {ITEM_RUNE_HELM, 400},
    {ITEM_MINER_RING, 100},
    {ITEM_CORE_SIGIL, 100},
};

#define DROPS(table) table, sizeof(table) / sizeof(table[0])

// Balance rule: each zone is tuned against the stats a player typically arrives with
// (the previous zone's full set near the level cap, plus the combat upgrades bought by
// then), measured by simulation rather than assumed:
//
//   arrival (atk / def / hp):  Sewers 10/5/90 (starting kit)   Caves 21/34/119
//                              Crypt 70/162/523   Depths 400/575/1408   Core 1060/2790/3960
//
// Defense sits at 1.2-1.4x that attack, so the auto-attack is blocked on arrival while the
// strike lands for 60-80% of it. Attack costs roughly a tenth of max HP per hit, so a fresh
// arrival dies every four to six kills unless the guard is timed. Health makes an arrival
// fight last 10-18 s. The zone's own drops are what make it idle-farmable.
//
// The Sewers are set by hand: fought with the starting kit, they cost about 40% of health
// per fight and a death every three to four kills until the first drops come in.
//
// Gold per kill is about 5x the expected gold/s when the zone opens, so fighting actively
// adds roughly 40% on top of the buildings at that point and fades as production grows.
static constexpr EnemyDef SEWER_ENEMIES[] = {
    //  name             hp      atk   def   interval  gold      sprite        drops
    {"Sewer Rat",        30,     10,   12,   2400,     5,        RAT_SPRITE,   DROPS(SEWER_RAT_DROPS)},
    {"Sewer Slime",      36,     12,   13,   2800,     8,        RAT_SPRITE,   DROPS(SEWER_SLIME_DROPS)},
};
static constexpr EnemyDef CAVE_ENEMIES[] = {
    {"Cave Bat",         71,     49,   25,   2400,     800,      RAT_SPRITE,   DROPS(CAVE_BAT_DROPS)},
    {"Rock Crawler",     80,     57,   29,   2600,     1300,     RAT_SPRITE,   DROPS(ROCK_CRAWLER_DROPS)},
};
static constexpr EnemyDef CRYPT_ENEMIES[] = {
    {"Bone Digger",      240,    210,  84,   2400,     30000,    RAT_SPRITE,   DROPS(BONE_DIGGER_DROPS)},
    {"Crypt Ghoul",      270,    240,  98,   2600,     45000,    RAT_SPRITE,   DROPS(CRYPT_GHOUL_DROPS)},
};
static constexpr EnemyDef DEPTH_ENEMIES[] = {
    {"Deep Lurker",      1400,   690,  480,  2400,     250000,   RAT_SPRITE,   DROPS(DEEP_LURKER_DROPS)},
    {"Abyss Worm",       1500,   740,  560,  2600,     400000,   RAT_SPRITE,   DROPS(ABYSS_WORM_DROPS)},
};
static constexpr EnemyDef CORE_ENEMIES[] = {
    {"Magma Hound",      3600,   3100, 1300, 2400,     4000000,  RAT_SPRITE,   DROPS(MAGMA_HOUND_DROPS)},
    {"Core Golem",       4000,   3200, 1500, 2600,     6000000,  RAT_SPRITE,   DROPS(CORE_GOLEM_DROPS)},
};

#define ZONE(table) table, sizeof(table) / sizeof(table[0])

// Zone indices line up with the bonusPercent of the UNLOCK_ZONE upgrades in GameConfig.h
static constexpr ZoneDef ZONES[] = {
    //  name           enemies                   needsUpgrade  prevZoneKills  icon
    {"Sewers",      ZONE(SEWER_ENEMIES),       false,        0,             image_zone_sewers_pixels},
    {"Caves",       ZONE(CAVE_ENEMIES),        true,         25,            image_zone_caves_pixels},
    {"Crypt",       ZONE(CRYPT_ENEMIES),       true,         50,            image_zone_crypt_pixels},
    {"Depths",      ZONE(DEPTH_ENEMIES),       true,         100,           image_zone_depths_pixels},
    {"Molten Core", ZONE(CORE_ENEMIES),        true,         150,           image_zone_core_pixels},
};
static constexpr uint8_t ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

#undef RAT_SPRITE
#undef DROPS
#undef ZONE

#endif // COMBAT_CONFIG_H
