#ifndef COMBAT_STATE_H
#define COMBAT_STATE_H

#include <Arduino.h>
#include "CombatConfig.h"
#include "Stats.h"
#include "AchievementConfig.h"

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

    // Which of the player's hits landed last, so the screen can name it
    enum HitKind : uint8_t
    {
        HIT_AUTO,
        HIT_STRIKE,
        HIT_CRIT // A strike through an open weak point
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
    bool strike(unsigned long now);
    bool guard(unsigned long now);
    bool castSpell(unsigned long now); // The equipped amulet's spell; see SPELLS

    // The spell the equipped amulet grants, or nullptr without one
    const SpellDef *getSpell() const;
    // Why castSpell() would refuse right now beyond the cooldown: full health for a heal,
    // a Midas already armed. Lets the screen grey the button out instead of erroring.
    bool isSpellUseless() const;
    bool isWarded(unsigned long now) const;
    unsigned long getWardRemaining(unsigned long now) const;
    bool isMidasArmed() const { return midasArmed; }

    // The worn relics' perks, capped; zero without one
    uint32_t getArmorBreakPerk() const;
    uint32_t getWeakPointPerk() const;
    uint32_t getFuryPerk() const;
    uint8_t getArmorBreakPercent() const { return armorBreak; } // Taken off this enemy so far
    uint32_t getEnemyDefense() const; // With the armor break applied
    bool isWeakPointOpen(unsigned long now) const;
    uint32_t getWeakPointEvents() const { return weakPointEvents; }
    uint8_t getFuryPercent() const { return (uint8_t)(fury / FURY_FULL); }
    bool isFuryActive(unsigned long now) const;
    unsigned long getFuryRemaining(unsigned long now) const;
    uint32_t getFuryEvents() const { return furyEvents; }

    bool enterZone(uint8_t zoneId, unsigned long now);
    void leaveZone();
    // Swaps a fight in progress for a fresh, different enemy. A death or a respawn already
    // under way is left to run, so this never skips the revive wait.
    void rerollEnemy(unsigned long now);
    bool isZoneUnlocked(uint8_t zoneId) const;
    // False for a relic the Relic Sense talent has not reached yet
    bool canDrop(uint8_t itemId) const;

    // The boss of the zone, if it has one, comes after this many more kills; 0 while it is up
    bool hasBoss() const { return bossIndex < ZONES[currentZone].enemyCount; }
    uint8_t getKillsToBoss() const { return killsToBoss; }
    bool isBossFight() const { return phase != IDLE && getEnemy().boss; }
    unsigned long getBossSwiftRemaining(unsigned long now) const; // 0 once the swift bonus is out of reach
    uint32_t getBossEvents() const { return bossEvents; }         // A boss stepped up
    uint32_t getBossFledEvents() const { return bossFledEvents; } // It killed the player and left
    bool wasLastKillSwift() const { return lastKillSwift; }
    uint32_t getBossKills() const { return bossKills; }

    // Second Wind turns a death into a revive at half health, then recharges
    bool isSecondWindReady(unsigned long now) const;
    unsigned long getSecondWindRemaining(unsigned long now) const;
    uint32_t getSecondWindEvents() const { return secondWindEvents; }

    Phase getPhase() const { return phase; }
    bool isInZone() const { return inZone; }
    uint8_t getCurrentZone() const { return currentZone; }
    const ZoneDef &getZone() const { return ZONES[currentZone]; }
    const EnemyDef &getEnemy() const { return ZONES[currentZone].enemies[currentEnemy]; }

    uint32_t getEnemyHp() const { return enemyHp; }
    uint32_t getPlayerHp() const { return playerHp; }
    const Stats &getPlayerStats() const { return cachedStats; }
    const Inventory &getInventory() const { return inventory; }

    // Zero means the attack cannot beat the enemy's defense, so the zone is out of reach
    uint32_t getAutoAttackDamage() const;
    uint32_t getStrikeDamage() const;
    // The best single hit the player can currently land; zero means the zone is out of reach
    uint32_t getBestAttackDamage() const;
    // What the enemy's next hit would take off, ignoring any active guard. Zero means the
    // player's defense shuts it out entirely.
    uint32_t getEnemyAttackDamage() const;
    // The same hit with an active guard applied; a ward is left to the caller
    uint32_t getEnemyHitDamage(unsigned long now) const;

    uint16_t getZoneKills(uint8_t zoneId) const;
    uint32_t getTotalKills() const; // Summed from the per-zone counts, so it persists with them

    // Milliseconds left, for the countdowns on the combat screen
    unsigned long getAutoAttackRemaining(unsigned long now) const;
    unsigned long getEnemyAttackRemaining(unsigned long now) const;
    unsigned long getStrikeCooldownRemaining(unsigned long now) const;
    unsigned long getSpellCooldownRemaining(unsigned long now) const;
    unsigned long getGuardCooldownRemaining(unsigned long now) const;
    unsigned long getReviveRemaining(unsigned long now) const;
    unsigned long getRespawnRemaining(unsigned long now) const;
    bool isGuarding(unsigned long now) const;
    // Base timings and odds with the achievement rewards and item cuts applied
    unsigned long getGuardDurationMs() const;
    unsigned long getStrikeCooldownMs() const;
    unsigned long getGuardCooldownMs() const;
    unsigned long getSpellCooldownMs(const SpellDef &spell) const;
    uint16_t getDropChanceBp(const DropDef &drop) const;
    unsigned long getGuardRemaining(unsigned long now) const; // 0 when not guarding

    // Monotonic counters the screen diffs against its own last-seen values, so it can
    // react to a kill or a drop without the engine knowing about the UI
    uint32_t getKillEvents() const { return killEvents; }
    uint64_t getLastKillGold() const { return lastKillGold; } // With any Midas multiplier
    // What killing `enemy` in `zone` pays now, before a Midas
    uint64_t getKillGold(const EnemyDef &enemy, uint8_t zone) const;
    bool wasLastKillMidas() const { return lastKillMidas; }
    // A hit on the enemy, and a hit on the player that did damage (after any guard)
    uint32_t getHitEvents() const { return hitEvents; }
    uint32_t getLastHitDamage() const { return lastHitDamage; }
    HitKind getLastHitKind() const { return lastHitKind; }
    uint32_t getHurtEvents() const { return hurtEvents; }
    uint32_t getLastHurtDamage() const { return lastHurtDamage; }
    bool wasLastHurtGuarded() const { return lastHurtGuarded; }
    // One event per kill that dropped anything, summed up by its rarest drop
    struct KillDrops
    {
        uint8_t itemId; // The rarest; a new item wins a tie
        bool isNew;
        uint8_t level;  // Level that item reached with the drop
        uint8_t count;  // Items dropped by the kill, that one included
        bool anyNew;
    };
    uint32_t getDropEvents() const { return dropEvents; }
    const KillDrops &getLastDrops() const { return lastDrops; }

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    uint32_t statsSignature() const;
    void refreshStats();
    void startFight(unsigned long now);
    void startRespawn(unsigned long now);
    void regenerate(unsigned long now);
    uint32_t getReward(AchReward reward) const;
    void pickEnemy(uint8_t avoid = 0xFF);
    void resetBossCount();
    void scheduleWeakPoint(unsigned long now);
    void updateWeakPoint(unsigned long now);
    void addFury(uint16_t amount, unsigned long now);
    uint32_t damageTo(uint32_t defense, uint8_t multiplier) const;
    void hitEnemy(uint32_t damage, HitKind kind, unsigned long now);
    void onEnemyKilled(unsigned long now);
    void onPlayerDied(unsigned long now);

    GameState &game;
    Inventory &inventory;

    uint8_t currentZone;
    bool inZone;
    uint8_t currentEnemy;
    uint32_t enemyHp;
    uint32_t playerHp;
    bool hpStarted;
    uint16_t zoneKills[ZONE_COUNT];
    Phase phase;

    Stats cachedStats;
    uint32_t seenStatsSignature;

    unsigned long autoAttackAt;
    unsigned long enemyAttackAt;
    unsigned long strikeReadyAt;
    unsigned long spellReadyAt;
    unsigned long wardEndsAt;
    bool midasArmed;
    unsigned long guardReadyAt;
    unsigned long guardEndsAt;
    unsigned long respawnAt;
    unsigned long reviveAt;
    unsigned long reviveMs; // This death's wait, which the health bar fills over
    unsigned long deathAt;
    unsigned long regenAt;
    uint8_t regenCarry; // Tenths of HP not yet credited

    uint32_t killEvents;
    uint64_t lastKillGold;
    bool lastKillMidas;
    uint32_t hitEvents;
    uint32_t lastHitDamage;
    HitKind lastHitKind;
    uint32_t hurtEvents;
    uint32_t lastHurtDamage;
    bool lastHurtGuarded;
    uint32_t dropEvents;
    KillDrops lastDrops;
    uint8_t bossIndex; // Past the zone's enemy count when it has no boss
    uint8_t killsToBoss;
    unsigned long bossStartedAt;
    uint32_t bossEvents;
    uint32_t bossFledEvents;
    bool lastKillSwift;
    uint32_t bossKills; // Since boot
    unsigned long secondWindReadyAt; // 0 until the first one is used
    uint32_t secondWindEvents;

    uint8_t armorBreak;
    unsigned long weakOpensAt;  // 0 while none is scheduled
    unsigned long weakClosesAt; // 0 while closed
    uint32_t weakPointEvents;
    uint16_t fury; // Hundredths of a meter point, FURY_FULL * 100 when full
    unsigned long furyEndsAt; // 0 outside a fury
    uint32_t furyEvents;
};

#endif // COMBAT_STATE_H
