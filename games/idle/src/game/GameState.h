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
    bool mine(); // Returns true when the click leveled up mining
    bool buyUpgrade(uint8_t id);

    // Ore
    uint64_t getOre(OreEnum ore) const; // Whole units
    uint32_t getProductionPerSecond(OreEnum ore) const; // ORE_SCALE units
    OreEnum getCurrentOre() const { return currentOre; }
    void setCurrentOre(OreEnum ore) { currentOre = ore; }

    // Upgrades
    uint16_t getUpgradeLevel(uint8_t id) const;
    uint64_t getUpgradeCost(uint8_t id, OreEnum ore) const; // Whole units
    bool canAfford(uint8_t id) const;

    // Mining progression
    uint16_t getMiningLevel() const { return miningLevel; }
    uint32_t getMiningXp() const { return miningXp; }
    uint32_t getXpToNextLevel() const { return XP_PER_LEVEL * miningLevel; }

private:
    uint64_t ore[ORE_COUNT];                // ORE_SCALE units
    uint32_t productionRemainder[ORE_COUNT]; // Leftover (units * ms) below one ORE_SCALE step
    uint16_t upgradeLevels[UPGRADE_COUNT];

    OreEnum currentOre;
    uint16_t miningLevel;
    uint32_t miningXp;

    bool started;
    unsigned long lastUpdate;

    void addOre(OreEnum type, uint64_t amount);
};

#endif // GAME_STATE_H
