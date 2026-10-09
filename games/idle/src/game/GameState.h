#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <Arduino.h>
#include "GameConfig.h"

class Inventory;
class Achievements;
class Ascension;

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
        uint32_t veinsCaught;
    };

    GameState(Inventory &inventory, Achievements &achievements, const Ascension &ascension);

    // Advances automatic production up to `now` (millis)
    void update(unsigned long now);

    // Actions
    // Returns the gold gained (whole units); leveledUp is set on a level-up
    uint32_t mine(bool &leveledUp, unsigned long now);
    uint32_t getComboPercent(unsigned long now) const; // Bonus the next click would get
    uint32_t getXpPerClick() const;
    bool buyBuilding(uint8_t id);
    bool buyGoldUpgrade(uint8_t id);
    void addGold(uint64_t amount); // Whole units; used by combat rewards
    const Achievements &getAchievements() const { return achievements; }
    const Ascension &getAscension() const { return ascension; }

    // Gold
    uint64_t getGold() const; // Whole units
    uint64_t getProductionPerSecond() const; // GOLD_SCALE units
    // One building's share of that, all its levels and one level, with the same bonuses applied
    uint64_t getBuildingProduction(uint8_t id) const;      // GOLD_SCALE units
    uint64_t getBuildingProductionPerLevel(uint8_t id) const; // GOLD_SCALE units
    uint32_t getBuildingMultiplier(uint8_t id) const; // From its BUILDING_BOOST upgrades: 1, 2, 4...
    uint32_t getClickAmount() const; // Whole units gained by the next mine(): ore tier, click upgrades, share of production

    // Buildings
    uint16_t getBuildingLevel(uint8_t id) const;
    uint64_t getBuildingCost(uint8_t id) const; // Whole units
    bool canAffordBuilding(uint8_t id) const;

    // Gold upgrades
    bool isGoldUpgradeBought(uint8_t id) const;
    bool isGoldUpgradeUnlocked(uint8_t id) const; // Requirements met, so the shop may show it
    bool canAffordGoldUpgrade(uint8_t id) const;
    uint64_t getGoldUpgradeCost(uint8_t id) const; // Whole units, after the worn relic's discount
    // Production folds in equipped items and unlocked achievements, click folds in items,
    // so gear and milestones both feed the economy
    uint32_t getProductionBonusPercent() const;
    // Points earned and production talents, applied on top of the bonus above
    uint32_t getAscensionProductionPercent() const;
    uint32_t getClickBonusPercent() const;
    // Share of production per second that every click adds on top (CLICK_PRODUCTION
    // upgrades), which keeps clicking worth it once buildings dwarf the ore's own value
    uint32_t getClickProductionPercent() const;

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

    // Gold veins (see VeinEffect). Only the count caught is saved; a vein on screen or a
    // bonus running is lost on a reboot, like a fight in progress.
    bool isVeinVisible() const { return veinVisible; }
    unsigned long getVeinRemaining(unsigned long now) const;
    VeinEffect catchVein(unsigned long now); // NONE when there is no vein to catch
    uint64_t getLastLuckyGold() const { return lastLuckyGold; } // Whole units
    VeinEffect getActiveBuff() const { return buff; }
    unsigned long getBuffRemaining(unsigned long now) const;
    uint32_t getVeinsCaught() const { return veinsCaught; }
    // Bumped each time a vein appears, so the Mining screen can chime once per vein
    uint32_t getVeinSpawns() const { return veinSpawns; }
    uint32_t getVeinFrequencyPercent() const;
    uint32_t getVeinPowerPercent() const;

    // Lifetime totals, which only achievements read today
    uint32_t getTotalClicks() const { return totalClicks; }
    uint64_t getTotalGoldEarned() const; // Whole units
    uint32_t getTotalBuildingLevels() const;
    uint8_t getUpgradesBoughtCount() const;

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    uint32_t getBonusPercent(UpgradeTarget target) const;
    void countBoosts(uint8_t *boosts) const; // BUILDING_COUNT entries
    uint64_t getBaseProductionPerSecond() const; // Without a vein's frenzy
    uint64_t applyProductionBonus(uint64_t production) const;
    uint64_t getBuildingBaseOutput(uint8_t id, const uint8_t *boosts) const;
    void applyHeadStart();
    void updateVeins(unsigned long now);
    void scheduleVein(unsigned long now);
    void creditGold(uint64_t scaledAmount); // Single place where gold comes in

    Inventory &inventory;
    Achievements &achievements;
    const Ascension &ascension;

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
    uint32_t seenAscensionRevision;

    unsigned long lastMineAt;
    uint16_t comboCount; // Clicks in the current streak, runtime only

    bool veinVisible;
    unsigned long veinAt; // Next vein while none is up, else when the visible one goes
    VeinEffect buff;
    unsigned long buffUntil;
    uint64_t lastLuckyGold;
    uint32_t veinsCaught;
    uint32_t veinSpawns;
};

#endif // GAME_STATE_H
