#ifndef FOCUS_STATS_H
#define FOCUS_STATS_H

#include <Arduino.h>

// Finished focuses per day for the last week, the streak and the totals. Days are Clock::today()
// numbers; without a synced clock (day 0) a focus counts toward the totals and the session only.
class FocusStats
{
public:
    static const uint8_t DAYS = 7;

    struct Snapshot
    {
        uint16_t counts[DAYS]; // Indexed by day % DAYS, valid for the DAYS days up to lastDay
        uint16_t streak;
        uint32_t lastDay;
        uint32_t streakDay; // Last day with a focus
        uint32_t totalFocuses;
        uint32_t totalMinutes;
    };

    void record(uint8_t minutes, uint32_t day);

    uint16_t getCount(uint32_t day) const;
    // The run of days with a focus that ends today or yesterday
    uint16_t getStreak(uint32_t today) const;
    uint32_t getTotalFocuses() const { return data.totalFocuses; }
    uint32_t getTotalMinutes() const { return data.totalMinutes; }
    uint16_t getSessionFocuses() const { return sessionFocuses; }
    uint32_t getRevision() const { return revision; }

    void save(Snapshot &out) const { out = data; }
    void load(const Snapshot &in) { data = in; }

private:
    Snapshot data = {};
    uint16_t sessionFocuses = 0;
    uint32_t revision = 0;
};

#endif // FOCUS_STATS_H
