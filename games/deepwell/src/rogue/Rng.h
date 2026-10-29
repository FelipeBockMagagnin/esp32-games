#ifndef ROGUE_RNG_H
#define ROGUE_RNG_H

#include <stdint.h>

// xorshift32: a floor is regenerated from its seed after a reboot, so its rolls must not
// come from Arduino's random(), which is seeded from the hardware
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

#endif // ROGUE_RNG_H
