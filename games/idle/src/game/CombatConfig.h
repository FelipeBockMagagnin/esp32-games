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

static constexpr EnemyDef SEWER_ENEMIES[] = {
    //  name             hp      atk   def   interval  gold    sprite        drops
    {"Sewer Rat",        30,     4,    0,    2500,     5,      RAT_SPRITE,   DROPS(SEWER_RAT_DROPS)},
    {"Sewer Slime",      50,     6,    1,    2600,     10,     RAT_SPRITE,   DROPS(SEWER_SLIME_DROPS)},
};

static constexpr EnemyDef CAVE_ENEMIES[] = {
    {"Cave Bat",         150,    12,   3,    2200,     40,     RAT_SPRITE,   DROPS(CAVE_BAT_DROPS)},
    {"Rock Crawler",     250,    18,   8,    2600,     80,     RAT_SPRITE,   DROPS(ROCK_CRAWLER_DROPS)},
};

static constexpr EnemyDef CRYPT_ENEMIES[] = {
    {"Bone Digger",      900,    45,   25,   2200,     400,    RAT_SPRITE,   DROPS(BONE_DIGGER_DROPS)},
    {"Crypt Ghoul",      1400,   60,   35,   2400,     700,    RAT_SPRITE,   DROPS(CRYPT_GHOUL_DROPS)},
};

static constexpr EnemyDef DEPTH_ENEMIES[] = {
    {"Deep Lurker",      5000,   180,  110,  2000,     3000,   RAT_SPRITE,   DROPS(DEEP_LURKER_DROPS)},
    {"Abyss Worm",       8000,   240,  150,  2200,     6000,   RAT_SPRITE,   DROPS(ABYSS_WORM_DROPS)},
};

static constexpr EnemyDef CORE_ENEMIES[] = {
    {"Magma Hound",      30000,  700,  450,  1900,     25000,  RAT_SPRITE,   DROPS(MAGMA_HOUND_DROPS)},
    {"Core Golem",       55000,  950,  600,  2100,     50000,  RAT_SPRITE,   DROPS(CORE_GOLEM_DROPS)},
};

#define ZONE(table) table, sizeof(table) / sizeof(table[0])

// Zone indices line up with the bonusPercent of the UNLOCK_ZONE upgrades in GameConfig.h
static constexpr ZoneDef ZONES[] = {
    //  name           enemies                   needsUpgrade  prevZoneKills
    {"Sewers",      ZONE(SEWER_ENEMIES),       false,        0},
    {"Caves",       ZONE(CAVE_ENEMIES),        true,         10},
    {"Crypt",       ZONE(CRYPT_ENEMIES),       true,         25},
    {"Depths",      ZONE(DEPTH_ENEMIES),       true,         50},
    {"Molten Core", ZONE(CORE_ENEMIES),        true,         100},
};
static constexpr uint8_t ZONE_COUNT = sizeof(ZONES) / sizeof(ZONES[0]);

#undef RAT_SPRITE
#undef DROPS
#undef ZONE

#endif // COMBAT_CONFIG_H
