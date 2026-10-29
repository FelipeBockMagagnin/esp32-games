#ifndef PET_RNG_H
#define PET_RNG_H

#include <stdint.h>

// xorshift32, so the simulator replays the same days from a seed; never Arduino's random()
class Rng
{
public:
    explicit Rng(uint32_t seed) : state(seed != 0 ? seed : 0x9E3779B9u) {}

    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    // [0, max)
    uint32_t below(uint32_t max) { return max > 0 ? next() % max : 0; }

    // [min, max]
    int32_t range(int32_t min, int32_t max) { return min + (int32_t)below((uint32_t)(max - min + 1)); }

private:
    uint32_t state;
};

#endif // PET_RNG_H
