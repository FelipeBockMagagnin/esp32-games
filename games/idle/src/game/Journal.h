#ifndef JOURNAL_H
#define JOURNAL_H

#include <Arduino.h>

class GameState;
class CombatState;

// Records the Journal screen shows that a run's own state loses on ascending: the past runs'
// totals, the best streak, time played, and the last few ascensions. Kept across ascensions
// like the awards. Holds no references, like Achievements: update() and bankRun() take the
// states they read.
class Journal
{
public:
    static const uint8_t HISTORY = 5;

    struct RunRecord
    {
        uint32_t seconds;
        uint32_t points;
    };

    struct Snapshot
    {
        uint32_t runSeconds; // Played this run, while powered on
        uint32_t pastSeconds;
        uint32_t pastClicks;
        uint32_t pastKills;
        uint32_t pastVeins;
        uint32_t bossKills; // Every run
        uint16_t bestStreak;
        uint8_t runCount;   // History entries filled
        RunRecord history[HISTORY]; // Newest first
    };

    Journal();

    void update(unsigned long now, const GameState &game, const CombatState &combat);
    // Folds the run into the totals and the history, before the ascension clears it
    void bankRun(const GameState &game, const CombatState &combat, uint32_t points);

    uint32_t getRunSeconds() const { return data.runSeconds; }
    uint32_t getTotalSeconds() const { return data.pastSeconds + data.runSeconds; }
    uint32_t getLifetimeClicks(const GameState &game) const;
    uint32_t getLifetimeKills(const CombatState &combat) const;
    uint32_t getLifetimeVeins(const GameState &game) const;
    uint32_t getBossKills() const { return data.bossKills; }
    uint16_t getBestStreak() const { return data.bestStreak; }
    uint8_t getRunCount() const { return data.runCount; }
    const RunRecord &getRun(uint8_t newest) const { return data.history[newest]; }

    void save(Snapshot &out) const { out = data; }
    void load(const Snapshot &in);

private:
    Snapshot data;
    unsigned long lastTick;
    bool started;
    uint32_t seenBossKills; // CombatState's count since boot
};

#endif // JOURNAL_H
