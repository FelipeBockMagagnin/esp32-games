#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

#include <Arduino.h>
#include "../enum/OreEnum.h"

// Ore amounts are stored in thousandths so fractional production (e.g. 0.1/s) stays exact
static const uint32_t ORE_SCALE = 1000;
static const uint8_t ORE_COUNT = static_cast<uint8_t>(OreEnum::COUNT);

// Manual mining
static const uint32_t MINE_AMOUNT = 1 * ORE_SCALE; // Ore gained per button click
static const uint32_t MINE_XP = 1;                 // Mining XP gained per click
static const uint32_t XP_PER_LEVEL = 10;           // XP needed for next level = XP_PER_LEVEL * level

// Upgrades that generate ore automatically
struct UpgradeDef
{
    const char *name;
    OreEnum producedOre;
    uint32_t productionPerLevel;    // Per second, in ORE_SCALE units (100 = 0.1/s)
    uint32_t baseCost[ORE_COUNT];   // Whole ore units, indexed by OreEnum
    uint16_t costGrowthPercent;     // Cost multiplier per level bought (115 = +15%)
};

// Keep in sync with the UPGRADES table in GameConfig.cpp
static const uint8_t UPGRADE_COUNT = 4;
extern const UpgradeDef UPGRADES[UPGRADE_COUNT];

#endif // GAME_CONFIG_H
