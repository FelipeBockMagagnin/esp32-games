#ifndef ASCENSION_H
#define ASCENSION_H

#include <Arduino.h>
#include "AscensionConfig.h"

// What survives an ascension besides the awards and the items: the gold of every past run,
// which the points are counted from, and the talents bought with them. Like Achievements it
// holds no references; callers pass the current run's gold (GameState::getTotalGoldEarned).
class Ascension
{
public:
    struct Snapshot
    {
        uint64_t priorGold; // Whole units earned by the runs before this one
        uint16_t ascensions;
        uint8_t talentLevels[TALENT_COUNT];
    };

    Ascension();

    static uint32_t pointsFor(uint64_t lifetimeGold);

    uint64_t getLifetimeGold(uint64_t runGold) const;
    uint32_t getPointsEarned() const { return pointsFor(priorGold); }
    uint32_t getPointsAvailable() const;
    // What ascending now would add, and the run gold the next one of those needs
    uint32_t getPendingPoints(uint64_t runGold) const;
    uint64_t getRunGoldForNextPoint(uint64_t runGold) const;
    uint16_t getAscensions() const { return ascensions; }

    // Banks the run's gold; the caller saves and restarts into a fresh run
    bool ascend(uint64_t runGold);

    uint8_t getTalentLevel(uint8_t id) const;
    bool isTalentOpen(uint8_t id) const; // Its parent has a level
    bool isTalentMaxed(uint8_t id) const;
    uint32_t getTalentCost(uint8_t id) const; // Of the next level
    bool canBuyTalent(uint8_t id) const;
    bool buyTalent(uint8_t id);
    uint8_t getTalentDepth(uint8_t id) const;

    // Summed per-level values (a percent for most effects) and summed levels
    uint32_t getPercent(TalentEffect effect) const { return effectPercent[(uint8_t)effect]; }
    uint8_t getEffectLevel(TalentEffect effect) const { return effectLevel[(uint8_t)effect]; }

    // Bumped on every change, so combat knows to recompute the player's stats
    uint32_t getRevision() const { return revision; }

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    void recount();

    uint64_t priorGold;
    uint16_t ascensions;
    uint8_t talentLevels[TALENT_COUNT];
    uint32_t pointsSpent;
    uint32_t effectPercent[TALENT_EFFECT_COUNT];
    uint8_t effectLevel[TALENT_EFFECT_COUNT];
    uint32_t revision;
};

#endif // ASCENSION_H
