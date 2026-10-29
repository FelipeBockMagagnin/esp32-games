#ifndef DICE_RANDOM_H
#define DICE_RANDOM_H

#include <stdint.h>

// Every roll, draw and spin takes its numbers from here: esp_random() on the board, a seeded
// generator in the tests
using RandomSource = uint32_t (*)();

// [0, n) with every value equally likely: a plain `% n` favours the low values whenever n does
// not divide 2^32, so the 2^32 % n raw values below the threshold are drawn again
inline uint32_t uniform(RandomSource source, uint32_t n)
{
    if (n <= 1)
    {
        return 0;
    }
    uint32_t threshold = (0u - n) % n;
    uint32_t r;
    do
    {
        r = source();
    } while (r < threshold);
    return r % n;
}

#endif // DICE_RANDOM_H
