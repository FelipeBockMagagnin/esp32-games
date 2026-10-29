#include "Spinner.h"
#include <math.h>

// The wheel loses this share of its speed each ms, and halts below STOP_SPEED: about 4 s
// from launch to rest for the 4 to 6 turns a spin covers
static const float FRICTION = 0.0012f;
static const float STOP_SPEED = 0.02f;
static const uint8_t MIN_TURNS = 4;
static const uint8_t EXTRA_TURNS = 3;
// How far into the winning slice it may stop, so it never rests on an edge
static const uint32_t LANDING_STEPS = 1000;
static const float LANDING_MARGIN = 0.15f;

static float wrap(float degrees)
{
    float w = fmodf(degrees, 360.0f);
    return w < 0 ? w + 360.0f : w;
}

void Spinner::setSlices(uint8_t count)
{
    if (count < MIN_SLICES || count > MAX_SLICES)
    {
        count = DEFAULT_SLICES;
    }
    if (count == slices)
    {
        return;
    }
    slices = count;
    spinning = false;
    angle = 360.0f - 180.0f / slices;
    result = 0;
}

uint8_t Spinner::sliceAtPointer() const
{
    uint8_t slice = (uint8_t)(wrap(-angle) / sliceDegrees());
    return slice < slices ? slice : slices - 1;
}

void Spinner::spin(unsigned long now, RandomSource source)
{
    result = (uint8_t)uniform(source, slices);
    float into = LANDING_MARGIN + (1.0f - 2 * LANDING_MARGIN) * uniform(source, LANDING_STEPS) / (LANDING_STEPS - 1);
    float landing = (result + into) * sliceDegrees();
    uint8_t turns = MIN_TURNS + (uint8_t)uniform(source, EXTRA_TURNS);

    startAngle = angle;
    travel = 360.0f * turns + wrap(-landing - angle);
    launchSpeed = travel * FRICTION + STOP_SPEED;
    duration = (unsigned long)(logf(launchSpeed / STOP_SPEED) / FRICTION);
    startedAt = now;
    crossings = 0;
    spinning = true;
}

void Spinner::place(float unwrapped)
{
    float width = sliceDegrees();
    crossings = (uint16_t)(floorf(unwrapped / width) - floorf(startAngle / width));
    angle = wrap(unwrapped);
}

void Spinner::update(unsigned long now)
{
    if (!spinning)
    {
        return;
    }
    unsigned long elapsed = now - startedAt;
    if (elapsed >= duration)
    {
        stop();
        return;
    }
    place(startAngle + launchSpeed / FRICTION * (1.0f - expf(-FRICTION * elapsed)));
}

void Spinner::stop()
{
    if (!spinning)
    {
        return;
    }
    place(startAngle + travel);
    spinning = false;
}
