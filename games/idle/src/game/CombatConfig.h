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
    {ITEM_RUSTY_SWORD, 400},
    {ITEM_CLOTH_HOOD, 500},
    {ITEM_RAG_GLOVES, 500},
};
static constexpr DropDef SEWER_SLIME_DROPS[] = {
    {ITEM_CLOTH_VEST, 500},
    {ITEM_CLOTH_PANTS, 500},
    {ITEM_WORN_BOOTS, 500},
    {ITEM_WOOD_SHIELD, 300},
    {ITEM_TIN_RING, 120},
    {ITEM_BONE_CHARM, 120},
};

static constexpr DropDef CAVE_BAT_DROPS[] = {
    {ITEM_IRON_HELM, 450},
    {ITEM_IRON_GLOVES, 450},
    {ITEM_IRON_SWORD, 350},
};
static constexpr DropDef ROCK_CRAWLER_DROPS[] = {
    {ITEM_IRON_MAIL, 450},
    {ITEM_IRON_LEGS, 450},
    {ITEM_IRON_BOOTS, 450},
    {ITEM_IRON_SHIELD, 300},
    {ITEM_GOLD_RING, 100},
    {ITEM_JADE_AMULET, 100},
};

static constexpr DropDef BONE_DIGGER_DROPS[] = {
    {ITEM_STEEL_HELM, 400},
    {ITEM_STEEL_GRIPS, 400},
    {ITEM_STEEL_SWORD, 300},
};
static constexpr DropDef CRYPT_GHOUL_DROPS[] = {
    {ITEM_STEEL_PLATE, 400},
    {ITEM_STEEL_LEGS, 400},
    {ITEM_STEEL_BOOTS, 400},
    {ITEM_STEEL_GUARD, 250},
    {ITEM_RUBY_RING, 80},
    {ITEM_GOLD_AMULET, 80},
};

static constexpr DropDef DEEP_LURKER_DROPS[] = {
    {ITEM_RUNE_HELM, 300},
    {ITEM_RUNE_GRIPS, 300},
    {ITEM_RUNE_BLADE, 200},
};
static constexpr DropDef ABYSS_WORM_DROPS[] = {
    {ITEM_RUNE_PLATE, 300},
    {ITEM_RUNE_LEGS, 300},
    {ITEM_RUNE_BOOTS, 300},
    {ITEM_RUNE_AEGIS, 200},
};

static constexpr DropDef MAGMA_HOUND_DROPS[] = {
    {ITEM_RUNE_BLADE, 400},
    {ITEM_RUNE_AEGIS, 400},
    {ITEM_MINER_RING, 60},
};
static constexpr DropDef CORE_GOLEM_DROPS[] = {
    {ITEM_RUNE_PLATE, 500},
    {ITEM_RUNE_HELM, 500},
    {ITEM_MINER_RING, 120},
    {ITEM_CORE_SIGIL, 120},
};

#define DROPS(table) table, sizeof(table) / sizeof(table[0])

// Balance rule: each zone's defense sits ABOVE the attack of the gear the player arrives
// with, so the auto-attack alone does nothing, but BELOW twice it, so the manual strike
// still breaks through. Equipping and levelling items is what turns a zone idle-farmable.
//
// Player attack with a full set at item level 1: 10 starting (sword only), then 13 / 29 /
// 77 / 221 for gear tiers 1-4. Player defense: 7 starting, then 20 / 58 / 172 / 514.
static constexpr EnemyDef SEWER_ENEMIES[] = {
    //  name             hp      atk   def   interval  gold    sprite        drops
    {"Sewer Rat",        30,     10,   11,   2500,     10,     RAT_SPRITE,   DROPS(SEWER_RAT_DROPS)},
    {"Sewer Slime",      45,     12,   12,   2600,     20,     RAT_SPRITE,   DROPS(SEWER_SLIME_DROPS)},
};

static constexpr EnemyDef CAVE_ENEMIES[] = {
    {"Cave Bat",         250,    22,   18,   2200,     60,     RAT_SPRITE,   DROPS(CAVE_BAT_DROPS)},
    {"Rock Crawler",     400,    28,   22,   2600,     120,    RAT_SPRITE,   DROPS(ROCK_CRAWLER_DROPS)},
};

static constexpr EnemyDef CRYPT_ENEMIES[] = {
    {"Bone Digger",      1500,   70,   35,   2200,     600,    RAT_SPRITE,   DROPS(BONE_DIGGER_DROPS)},
    {"Crypt Ghoul",      2200,   90,   45,   2400,     1000,   RAT_SPRITE,   DROPS(CRYPT_GHOUL_DROPS)},
};

static constexpr EnemyDef DEPTH_ENEMIES[] = {
    {"Deep Lurker",      8000,   200,  90,   2000,     4500,   RAT_SPRITE,   DROPS(DEEP_LURKER_DROPS)},
    {"Abyss Worm",       12000,  260,  115,  2200,     9000,   RAT_SPRITE,   DROPS(ABYSS_WORM_DROPS)},
};

static constexpr EnemyDef CORE_ENEMIES[] = {
    {"Magma Hound",      45000,  600,  250,  1900,     35000,  RAT_SPRITE,   DROPS(MAGMA_HOUND_DROPS)},
    {"Core Golem",       70000,  800,  320,  2100,     65000,  RAT_SPRITE,   DROPS(CORE_GOLEM_DROPS)},
};

#define ZONE(table) table, sizeof(table) / sizeof(table[0])

// Zone indices line up with the bonusPercent of the UNLOCK_ZONE upgrades in GameConfig.h
static constexpr ZoneDef ZONES[] = {
    //  name           enemies                   needsUpgrade  prevZoneKills  icon
    {"Sewers",      ZONE(SEWER_ENEMIES),       false,        0,             image_zone_sewers_pixels},
    {"Caves",       ZONE(CAVE_ENEMIES),        true,         10,            image_zone_caves_pixels},
    {"Crypt",       ZONE(CRYPT_ENEMIES),       true,         25,            image_zone_crypt_pixels},
    {"Depths",      ZONE(DEPTH_ENEMIES),       true,         50,            image_zone_depths_pixels},
    {"Molten Core", ZONE(CORE_ENEMIES),        true,         100,           image_zone_core_pixels},
};
static constexpr uint8_t ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

#undef RAT_SPRITE
#undef DROPS
#undef ZONE

#endif // COMBAT_CONFIG_H
