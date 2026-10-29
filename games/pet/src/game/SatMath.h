#ifndef SAT_MATH_H
#define SAT_MATH_H

#include <stdint.h>

// Late-game gold runs into the top of 64 bits, so its arithmetic saturates rather than wraps

inline uint64_t satAdd(uint64_t a, uint64_t b)
{
    return a > UINT64_MAX - b ? UINT64_MAX : a + b;
}

inline uint64_t satMul(uint64_t a, uint64_t b)
{
    return b != 0 && a > UINT64_MAX / b ? UINT64_MAX : a * b;
}

// a * b / c, dividing first once the product would not fit
inline uint64_t mulDiv(uint64_t a, uint64_t b, uint64_t c)
{
    if (b == 0 || a <= UINT64_MAX / b)
    {
        return a * b / c;
    }
    return satMul(a / c, b);
}

#endif // SAT_MATH_H
