#ifndef DICE_SPINNER_H
#define DICE_SPINNER_H

#include <stdint.h>
#include "Random.h"

static const uint8_t MIN_SLICES = 2;
static const uint8_t MAX_SLICES = 12;
static const uint8_t DEFAULT_SLICES = 6;

// A wheel of equal slices under a pointer at the top. Slice i spans [i, i + 1) slice widths
// clockwise from the top while the wheel sits at angle 0; the wheel turns clockwise.
//
// The winner is drawn first, uniformly, and the spin is shaped to land on it: friction slows
// the wheel exponentially, so a launch speed covers a known distance, and that distance is
// picked to end a few turns later inside the winning slice. Where it stops never depends on
// the frame rate.
class Spinner
{
public:
    void setSlices(uint8_t count);
    uint8_t getSlices() const { return slices; }

    void spin(unsigned long now, RandomSource source);
    void update(unsigned long now);
    void stop();

    bool isSpinning() const { return spinning; }
    // Degrees the wheel has turned clockwise, [0, 360)
    float getAngle() const { return angle; }
    uint8_t sliceAtPointer() const;
    uint8_t getResult() const { return result; }
    // Slice edges that passed the pointer since the spin started, for the clicker
    uint16_t getCrossings() const { return crossings; }
    unsigned long getDuration() const { return duration; }

private:
    uint8_t slices = DEFAULT_SLICES;
    float angle = 360.0f - 180.0f / DEFAULT_SLICES; // Slice 1 centred under the pointer
    bool spinning = false;
    uint8_t result = 0;
    uint16_t crossings = 0;

    float startAngle = 0;
    float travel = 0;
    float launchSpeed = 0; // Degrees per ms
    unsigned long startedAt = 0;
    unsigned long duration = 0;

    float sliceDegrees() const { return 360.0f / slices; }
    void place(float unwrapped);
};

#endif // DICE_SPINNER_H
