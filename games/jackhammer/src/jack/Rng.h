#ifndef JACK_RNG_H
#define JACK_RNG_H

#include <stdint.h>

// xorshift32, so a round replays from its seed on the PC as on the board
class Rng
{
public:
    explicit Rng(uint32_t seed = 1) { reseed(seed); }

    void reseed(uint32_t seed) { state = seed != 0 ? seed : 0x9E3779B9u; }

    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    uint32_t below(uint32_t n) { return n > 0 ? next() % n : 0; }

    float unit() { return (next() >> 8) * (1.0f / 16777216.0f); }

private:
    uint32_t state;
};

#endif // JACK_RNG_H
