#ifndef UNLOCKS_H
#define UNLOCKS_H

#include <Arduino.h>

class GameState;
class Inventory;
class CombatState;
class Achievements;

// The screens a new player has not reached yet. A fresh game shows only Mining, and
// each of these opens when the player first has a use for it, one at a time, so the
// game introduces itself a piece at a time instead of all eight screens on boot.
// Declared in unlock order: each one also waits for the one before it.
enum class Feature : uint8_t
{
    BUILDINGS, // Enough gold earned to buy the first building
    UPGRADES,  // A few buildings owned and an upgrade affordable
    AWARDS,    // First achievement earned
    COMBAT,    // A Drill owned; opens the Zones and Combat screens together
    INVENTORY, // First item dropped beyond the starting kit
    ASCEND     // Enough lifetime gold for the first ascension point
};
static constexpr uint8_t FEATURE_COUNT = 6;

// Like Achievements, holds no references: evaluate() takes the states it reads, and
// the main loop maps features to screens, so nothing here knows about the UI.
class Unlocks
{
public:
    struct Snapshot
    {
        bool unlocked[FEATURE_COUNT];
    };

    Unlocks();

    // Self-throttled; safe to call every loop. While noticeBusy (another notice owns the
    // header) a fresh unlock still takes effect, but its notice waits its turn.
    void evaluate(unsigned long now, const GameState &game, const Inventory &inventory,
                  const CombatState &combat, const Achievements &achievements, bool noticeBusy);

    bool isUnlocked(Feature feature) const { return unlocked[(uint8_t)feature]; }

    // Bumped on every unlock, so the main loop can re-apply screen locks only on a change
    uint8_t getRevision() const { return revision; }

    // Name of the newly opened screen while its notice is up, or nullptr
    const char *getNotice(unsigned long now) const;

    // What the player should do to open the next screen, and how close they are, so a new
    // player always has a goal on screen. Refreshed by evaluate(); runtime only, not saved.
    struct Goal
    {
        char label[40]; // Fits a full-width bar at text size 1
        uint32_t done;
        uint32_t target; // 0 when there is nothing to measure, which leaves the bar empty
    };
    // nullptr once everything is open
    const Goal *getGoal() const { return hasGoal ? &goal : nullptr; }

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    void refreshGoal(const GameState &game, const Inventory &inventory, const CombatState &combat,
                     const Achievements &achievements);
    bool isConditionMet(Feature feature, const GameState &game, const Inventory &inventory,
                        const CombatState &combat, const Achievements &achievements) const;

    bool unlocked[FEATURE_COUNT];
    uint8_t revision;

    unsigned long lastCheck;
    int8_t pendingNotice; // Feature whose notice is waiting for the header, or -1
    const char *noticeName;
    unsigned long noticeUntil;

    Goal goal;
    bool hasGoal;
};

#endif // UNLOCKS_H
