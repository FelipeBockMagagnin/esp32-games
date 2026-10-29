#ifndef DICE_RAFFLE_H
#define DICE_RAFFLE_H

#include <stdint.h>
#include "Random.h"

static const int16_t RAFFLE_MIN = 0;
static const int16_t RAFFLE_MAX = 9999;

// A number between two bounds. Without repeats it is a raffle or a bingo: every number comes
// out once, until the hat is empty. Changing the bounds or the rule puts every number back.
class Raffle
{
public:
    Raffle() { restart(); }

    void setRange(int16_t from, int16_t to);
    void setRepeats(bool allowed);
    void restart();

    int16_t getFrom() const { return from; }
    int16_t getTo() const { return to; }
    bool getRepeats() const { return repeats; }
    uint16_t size() const { return (uint16_t)(to - from + 1); }
    uint16_t getDrawnCount() const { return drawnCount; }
    uint16_t remaining() const { return repeats ? size() : (uint16_t)(size() - drawnCount); }
    bool exhausted() const { return !repeats && drawnCount >= size(); }

    // Restarts the hat first when it is empty
    int16_t draw(RandomSource source);

private:
    int16_t from = 1;
    int16_t to = 100;
    bool repeats = false;
    uint16_t drawnCount = 0;
    uint8_t drawn[(RAFFLE_MAX - RAFFLE_MIN + 1 + 7) / 8];

    bool isDrawn(int16_t value) const
    {
        uint16_t bit = (uint16_t)(value - RAFFLE_MIN);
        return drawn[bit >> 3] & (1 << (bit & 7));
    }
};

#endif // DICE_RAFFLE_H
