#include "Raffle.h"
#include <string.h>

void Raffle::setRange(int16_t newFrom, int16_t newTo)
{
    if (newFrom < RAFFLE_MIN)
    {
        newFrom = RAFFLE_MIN;
    }
    if (newTo > RAFFLE_MAX)
    {
        newTo = RAFFLE_MAX;
    }
    if (newTo < newFrom)
    {
        newTo = newFrom;
    }
    if (newFrom == from && newTo == to)
    {
        return;
    }
    from = newFrom;
    to = newTo;
    restart();
}

void Raffle::setRepeats(bool allowed)
{
    if (allowed != repeats)
    {
        repeats = allowed;
        restart();
    }
}

void Raffle::restart()
{
    memset(drawn, 0, sizeof(drawn));
    drawnCount = 0;
}

int16_t Raffle::draw(RandomSource source)
{
    if (repeats)
    {
        return (int16_t)(from + uniform(source, size()));
    }
    if (exhausted())
    {
        restart();
    }
    // The pick-th number still in the hat, so each one left is equally likely
    uint32_t pick = uniform(source, remaining());
    for (int16_t value = from; value <= to; value++)
    {
        if (isDrawn(value))
        {
            continue;
        }
        if (pick-- == 0)
        {
            uint16_t bit = (uint16_t)(value - RAFFLE_MIN);
            drawn[bit >> 3] |= (uint8_t)(1 << (bit & 7));
            drawnCount++;
            return value;
        }
    }
    return from;
}
