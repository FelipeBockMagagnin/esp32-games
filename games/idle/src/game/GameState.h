#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <Arduino.h>
#include "GameConfig.h"

// Owns all game progress. Updated every loop regardless of the visible screen;
// screens only read from it and call its actions.
class GameState
{
public:
    GameState();

    // Advances automatic production up to `now` (millis)
    void update(unsigned long now);

    // Actions
    uint32_t mine(bool &leveledUp); // Returns the gold gained (whole units); leveledUp is set on a level-up
    bool buyBuilding(uint8_t id);
    bool buyGoldUpgrade(uint8_t id);

    // Gold
    uint64_t getGold() const; // Whole units
    uint32_t getProductionPerSecond() const; // GOLD_SCALE units
    uint32_t getClickAmount() const; // Whole units gained by the next mine(), current tier + click upgrades

    // Buildings
    uint16_t getBuildingLevel(uint8_t id) const;
    uint64_t getBuildingCost(uint8_t id) const; // Whole units
    bool canAffordBuilding(uint8_t id) const;

    // Gold upgrades
    bool isGoldUpgradeBought(uint8_t id) const;
    bool canAffordGoldUpgrade(uint8_t id) const;
    uint32_t getProductionBonusPercent() const; // Sum of the bought production upgrade bonuses
    uint32_t getClickBonusPercent() const;      // Sum of the bought click upgrade bonuses

    // Mining progression
    uint16_t getMiningLevel() const { return miningLevel; }
    uint32_t getMiningXp() const { return miningXp; }
    uint32_t getXpToNextLevel() const { return XP_PER_LEVEL * miningLevel; }
    uint8_t getOreTier() const; // Index into ORE_TIERS, based on the mining level
    const char *getOreTierName() const { return ORE_TIERS[getOreTier()].name; }

private:
    uint64_t gold;                // GOLD_SCALE units
    uint32_t productionRemainder; // Leftover (units * ms) below one GOLD_SCALE step
    uint16_t buildingLevels[BUILDING_COUNT];
    bool goldUpgradesBought[GOLD_UPGRADE_COUNT];

    uint16_t miningLevel;
    uint32_t miningXp;

    bool started;
    unsigned long lastUpdate;
};

#endif // GAME_STATE_H
