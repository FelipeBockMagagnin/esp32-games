#ifndef DICE_HISTORY_H
#define DICE_HISTORY_H

#include <stdint.h>

enum class Tool : uint8_t
{
    ROLL,
    DRAW,
    WHEEL
};

enum class Mark : uint8_t
{
    PLAIN,
    GOOD, // A critical or the best possible roll
    BAD   // A fumble or the worst possible roll
};

struct HistoryEntry
{
    Tool tool;
    Mark mark;
    int16_t result;
    unsigned long at;
    char what[16];   // The formula, the range, the wheel
    char detail[32]; // The faces, or what is left in the hat
};

// The latest results of every tool, newest first. Kept in RAM only: it is a table's scratch
// pad, and writing flash on every roll would wear it.
class History
{
public:
    static const uint8_t CAPACITY = 30;

    void add(Tool tool, Mark mark, int16_t result, const char *what, const char *detail, unsigned long at);
    uint8_t count() const { return size; }
    // 0 is the newest
    const HistoryEntry &get(uint8_t index) const;
    // Grows with every add, so a screen can tell its list is stale
    uint32_t getVersion() const { return version; }

private:
    HistoryEntry entries[CAPACITY] = {};
    uint8_t head = 0; // Where the next entry goes
    uint8_t size = 0;
    uint32_t version = 0;
};

#endif // DICE_HISTORY_H
