#ifndef COMBAT_STATE_H
#define COMBAT_STATE_H

#include <Arduino.h>
#include "CombatConfig.h"
#include "Stats.h"

class GameState;
class Inventory;

// Runs the fight. Ticked from the main loop like GameState, so a zone keeps paying out
// while the player is on the mining or shop screens.
class CombatState
{
public:
    enum Phase : uint8_t
    {
        IDLE,       // Not in a zone
        FIGHTING,
        RESPAWNING, // Enemy dead, next one on its way
        DEAD        // Player down, health refilling
    };

    struct Snapshot
    {
        uint8_t currentZone;
        bool inZone;
        uint16_t zoneKills[ZONE_COUNT];
    };

    CombatState(GameState &game, Inventory &inventory);

    void update(unsigned long now);

    // Actions; all return false when still on cooldown, out of combat, or unavailable
    bool strike();
    bool guard();
    bool smite(); // Needs an amulet equipped

    bool isSmiteUnlocked() const;

    bool enterZone(uint8_t zoneId);
    void leaveZone();
    bool isZoneUnlocked(uint8_t zoneId) const;

    Phase getPhase() const { return phase; }
    bool isInZone() const { return inZone; }
    uint8_t getCurrentZone() const { return currentZone; }
    const ZoneDef &getZone() const { return ZONES[currentZone]; }
    const EnemyDef &getEnemy() const { return ZONES[currentZone].enemies[currentEnemy]; }

    uint32_t getEnemyHp() const { return enemyHp; }
    uint32_t getPlayerHp() const { return playerHp; }
    const Stats &getPlayerStats() const { return cachedStats; }

    // Zero means the attack cannot beat the enemy's defense, so the zone is out of reach
    uint32_t getAutoAttackDamage() const;
    uint32_t getStrikeDamage() const;
    uint32_t getSmiteDamage() const;
    // The best single hit the player can currently land; zero means the zone is out of reach
    uint32_t getBestAttackDamage() const;
    // What the enemy's next hit would take off, ignoring any active guard. Zero means the
    // player's defense shuts it out entirely.
    uint32_t getEnemyAttackDamage() const;

    uint16_t getZoneKills(uint8_t zoneId) const;
    uint32_t getTotalKills() const; // Summed from the per-zone counts, so it persists with them

    // Milliseconds left, for the countdowns on the combat screen
    unsigned long getAutoAttackRemaining(unsigned long now) const;
    unsigned long getEnemyAttackRemaining(unsigned long now) const;
    unsigned long getStrikeCooldownRemaining(unsigned long now) const;
    unsigned long getSmiteCooldownRemaining(unsigned long now) const;
    unsigned long getGuardCooldownRemaining(unsigned long now) const;
    unsigned long getReviveRemaining(unsigned long now) const;
    bool isGuarding(unsigned long now) const;

    // Monotonic counters the screen diffs against its own last-seen values, so it can
    // react to a kill or a drop without the engine knowing about the UI
    uint32_t getKillEvents() const { return killEvents; }
    uint32_t getDropEvents() const { return dropEvents; }
    int8_t getLastDrop() const { return lastDrop; }
    bool wasLastDropNew() const { return lastDropWasNew; }
    uint8_t getLastDropLevel() const { return lastDropLevel; }

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    void refreshStats();
    void startFight(unsigned long now);
    void pickEnemy();
    uint32_t damageTo(uint32_t defense, uint8_t multiplier) const;
    void onEnemyKilled(unsigned long now);
    void onPlayerDied(unsigned long now);

    GameState &game;
    Inventory &inventory;

    uint8_t currentZone;
    bool inZone;
    uint8_t currentEnemy;
    uint32_t enemyHp;
    uint32_t playerHp;
    uint16_t zoneKills[ZONE_COUNT];
    Phase phase;

    Stats cachedStats;
    uint32_t seenInventoryRevision;

    unsigned long autoAttackAt;
    unsigned long enemyAttackAt;
    unsigned long strikeReadyAt;
    unsigned long smiteReadyAt;
    unsigned long guardReadyAt;
    unsigned long guardEndsAt;
    unsigned long respawnAt;
    unsigned long reviveAt;
    unsigned long deathAt;

    uint32_t killEvents;
    uint32_t dropEvents;
    int8_t lastDrop;
    bool lastDropWasNew;
    uint8_t lastDropLevel; // Level the item reached with that drop
};

#endif // COMBAT_STATE_H
