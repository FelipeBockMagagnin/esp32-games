#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <Arduino.h>
#include "GameConfig.h"

class Inventory;
class Achievements;

// Owns all game progress. Updated every loop regardless of the visible screen;
// screens only read from it and call its actions.
class GameState
{
public:
    // Persistent fields only, all POD so the whole struct can be written to flash as-is.
    // Runtime timing (lastUpdate/started) is deliberately left out.
    struct Snapshot
    {
        uint64_t gold;
        uint32_t productionRemainder;
        uint16_t buildingLevels[BUILDING_COUNT];
        bool goldUpgradesBought[GOLD_UPGRADE_COUNT];
        uint16_t miningLevel;
        uint32_t miningXp;
        uint32_t totalClicks;
        uint64_t totalGoldEarned;
    };

    GameState(Inventory &inventory, Achievements &achievements);

    // Advances automatic production up to `now` (millis)
    void update(unsigned long now);

    // Actions
    uint32_t mine(bool &leveledUp); // Returns the gold gained (whole units); leveledUp is set on a level-up
    bool buyBuilding(uint8_t id);
    bool buyGoldUpgrade(uint8_t id);
    void addGold(uint64_t amount); // Whole units; used by combat rewards

    // Gold
    uint64_t getGold() const; // Whole units
    uint64_t getProductionPerSecond() const; // GOLD_SCALE units
    // One building's share of that, all its levels and one level, with the same bonuses applied
    uint64_t getBuildingProduction(uint8_t id) const;      // GOLD_SCALE units
    uint64_t getBuildingProductionPerLevel(uint8_t id) const; // GOLD_SCALE units
    uint32_t getClickAmount() const; // Whole units gained by the next mine(), current tier + click upgrades

    // Buildings
    uint16_t getBuildingLevel(uint8_t id) const;
    uint64_t getBuildingCost(uint8_t id) const; // Whole units
    bool canAffordBuilding(uint8_t id) const;

    // Gold upgrades
    bool isGoldUpgradeBought(uint8_t id) const;
    bool isGoldUpgradeUnlocked(uint8_t id) const; // Requirements met, so the shop may show it
    bool canAffordGoldUpgrade(uint8_t id) const;
    // Production folds in equipped items and unlocked achievements, click folds in items,
    // so gear and milestones both feed the economy
    uint32_t getProductionBonusPercent() const;
    uint32_t getClickBonusPercent() const;

    // Combat bonuses granted by upgrades; the rest of a player's stats come from equipment
    uint32_t getAttackBonusPercent() const;
    uint32_t getDefenseBonusPercent() const;
    uint32_t getMaxHpBonusPercent() const;
    bool isZoneUnlockedByUpgrade(uint8_t zoneId) const;

    // Mining progression
    uint16_t getMiningLevel() const { return miningLevel; }
    uint32_t getMiningXp() const { return miningXp; }
    uint32_t getXpToNextLevel() const { return XP_PER_LEVEL * miningLevel; }
    uint8_t getOreTier() const; // Index into ORE_TIERS, based on the mining level
    const char *getOreTierName() const { return ORE_TIERS[getOreTier()].name; }

    // Lifetime totals, which only achievements read today
    uint32_t getTotalClicks() const { return totalClicks; }
    uint64_t getTotalGoldEarned() const; // Whole units
    uint32_t getTotalBuildingLevels() const;
    uint8_t getUpgradesBoughtCount() const;

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    uint32_t getBonusPercent(UpgradeTarget target) const;
    void creditGold(uint64_t scaledAmount); // Single place where gold comes in

    Inventory &inventory;
    Achievements &achievements;

    uint64_t gold;                // GOLD_SCALE units
    uint32_t productionRemainder; // Leftover (units * ms) below one GOLD_SCALE step
    uint16_t buildingLevels[BUILDING_COUNT];
    bool goldUpgradesBought[GOLD_UPGRADE_COUNT];

    uint16_t miningLevel;
    uint32_t miningXp;

    uint32_t totalClicks;
    uint64_t totalGoldEarned; // GOLD_SCALE units, never spent down

    bool started;
    unsigned long lastUpdate;
};

#endif // GAME_STATE_H
