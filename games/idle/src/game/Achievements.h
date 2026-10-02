#ifndef ACHIEVEMENTS_H
#define ACHIEVEMENTS_H

#include <Arduino.h>
#include "AchievementConfig.h"

class GameState;
class Inventory;
class CombatState;

// Tracks which achievements are unlocked and the gold production they add together.
// Holds no references of its own, which is what lets GameState read its bonus without
// a dependency cycle: evaluate() takes the other states as arguments instead.
class Achievements
{
public:
    struct Snapshot
    {
        bool unlocked[ACHIEVEMENT_COUNT];
    };

    Achievements();

    // Self-throttled; safe to call every loop
    void evaluate(unsigned long now, const GameState &game, const Inventory &inventory,
                  const CombatState &combat);

    bool isUnlocked(uint8_t id) const;
    uint8_t getUnlockedCount() const { return unlockedCount; }
    uint32_t getProductionBonusPercent() const { return bonusPercent; }

    // Current value of the counter an achievement watches; it unlocks once this reaches
    // the def's amount. Lets a screen show how close a locked one is.
    uint64_t getProgress(const AchievementDef &def, const GameState &game, const Inventory &inventory,
                         const CombatState &combat) const;

    // Name of the achievement whose notice is still on screen, or nullptr when none.
    // Whichever screen is visible paints it over its own header.
    const char *getNotice(unsigned long now) const;

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    bool isConditionMet(const AchievementDef &def, const GameState &game,
                        const Inventory &inventory, const CombatState &combat) const;
    void unlock(uint8_t id, unsigned long now);
    void recount();

    bool unlocked[ACHIEVEMENT_COUNT];
    uint8_t unlockedCount;
    uint32_t bonusPercent; // Cached sum, so the production getter stays cheap

    unsigned long lastCheck;
    const char *noticeName;
    unsigned long noticeUntil;
};

#endif // ACHIEVEMENTS_H
