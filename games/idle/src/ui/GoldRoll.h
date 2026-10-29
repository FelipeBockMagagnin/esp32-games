#ifndef UI_GOLD_ROLL_H
#define UI_GOLD_ROLL_H

#include <Arduino.h>

// A balance that rolls toward the real one, up as gold comes in and down as a purchase
// spends it, closing a fraction of the gap each step
class GoldRoll
{
public:
    void snap(uint64_t value) { shown = value; }

    // True when the shown value moved
    bool step(unsigned long now, uint64_t target)
    {
        if (now - lastStep < STEP_MS || target == shown)
        {
            return false;
        }
        lastStep = now;
        uint64_t gap = target > shown ? target - shown : shown - target;
        uint64_t move = gap / DIVISOR > 0 ? gap / DIVISOR : gap;
        shown = target > shown ? shown + move : shown - move;
        return true;
    }

    uint64_t get() const { return shown; }

private:
    static const unsigned long STEP_MS = 33;
    static const uint8_t DIVISOR = 4;

    uint64_t shown = 0;
    unsigned long lastStep = 0;
};

#endif // UI_GOLD_ROLL_H
