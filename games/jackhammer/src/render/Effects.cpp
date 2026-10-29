#include "Effects.h"
#include <math.h>
#include <string.h>

static const uint16_t COIN_COLOR = 0xFE60;
static const uint8_t COIN_SIZE = 3;
static const float COIN_TARGET_X = ARENA_PX - 20;
static const float COIN_TARGET_Y = -40;
static const int8_t SHAKE_PX = 3;
// A particle is let go this far past the arena's edge, so a big one does not pop off
static const float MARGIN = 16;
static const float UNITS_TO_PX_PER_STEP = PX_PER_UNIT * STEP_SECONDS;

static uint16_t mix(uint16_t a, uint16_t b, float t)
{
    int r = (a >> 11) + (int)(((b >> 11) - (a >> 11)) * t);
    int g = ((a >> 5) & 63) + (int)((((b >> 5) & 63) - ((a >> 5) & 63)) * t);
    int bl = (a & 31) + (int)(((b & 31) - (a & 31)) * t);
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

void Effects::reset()
{
    memset(particles, 0, sizeof(particles));
    source = nullptr;
    shakeLeft = 0;
    shakeX = shakeY = 0;
    busy = false;
}

Effects::Particle *Effects::freeSlot()
{
    for (Particle &p : particles)
    {
        if (p.life == 0)
        {
            return &p;
        }
    }
    return nullptr;
}

void Effects::emit(const Emitter &e, float x, float y, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        Particle *p = freeSlot();
        if (p == nullptr)
        {
            return;
        }
        // The share of the 3D speed that crosses the screen: a cone along the camera's axis,
        // or any direction for a sphere
        float across;
        if (e.coneDegrees > 0)
        {
            across = sinf(rng.unit() * e.coneDegrees * 0.017453f);
        }
        else
        {
            float z = rng.unit() * 2 - 1;
            across = sqrtf(1 - z * z);
        }
        float speed = between(e.speedMin, e.speedMax) * across * UNITS_TO_PX_PER_STEP;
        float angle = rng.unit() * 6.2832f;
        float size = between(e.sizeMin, e.sizeMax) * PX_PER_UNIT;
        *p = {x,
              y,
              cosf(angle) * speed,
              sinf(angle) * speed,
              e.texture,
              mix(e.tintFrom, e.tintTo, rng.unit()),
              (uint16_t)(between(e.lifeMin, e.lifeMax) / STEP_SECONDS),
              (uint8_t)(size < 1 ? 1 : size + 0.5f),
              false};
    }
    busy = true;
}

void Effects::setSource(const Emitter *emitter, float x, float y)
{
    source = emitter;
    sourceX = x;
    sourceY = y;
    owed = 0;
    burstIn = 0;
}

void Effects::coins(float x, float y, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        Particle *p = freeSlot();
        if (p == nullptr)
        {
            return;
        }
        // A kick to the side first, then they home in on the counter
        *p = {x, y, (rng.unit() - 0.5f) * 4.0f, (rng.unit() - 0.5f) * 4.0f, &PARTICLE_SQUARE, COIN_COLOR, 90,
              COIN_SIZE, true};
    }
    busy = true;
}

void Effects::shake(uint8_t steps)
{
    shakeLeft = steps;
    busy = true;
}

void Effects::step()
{
    if (source != nullptr)
    {
        owed += between(source->rateMin, source->rateMax) * STEP_SECONDS;
        if (owed >= 1)
        {
            emit(*source, sourceX, sourceY, (uint8_t)owed);
            owed -= (uint8_t)owed;
        }
        if (source->burst > 0)
        {
            burstIn -= STEP_SECONDS;
            if (burstIn <= 0)
            {
                emit(*source, sourceX, sourceY, source->burst);
                burstIn += source->period;
            }
        }
    }
    if (!busy)
    {
        return;
    }

    bool any = false;
    for (Particle &p : particles)
    {
        if (p.life == 0)
        {
            continue;
        }
        if (p.coin)
        {
            float dx = COIN_TARGET_X - p.x, dy = COIN_TARGET_Y - p.y;
            float d = sqrtf(dx * dx + dy * dy);
            p.vx = p.vx * 0.85f + dx / d * 1.2f;
            p.vy = p.vy * 0.85f + dy / d * 1.2f;
        }
        p.x += p.vx;
        p.y += p.vy;
        p.life--;
        if (p.x < -MARGIN || p.x > ARENA_PX + MARGIN || p.y < -MARGIN || p.y > ARENA_PX + MARGIN)
        {
            p.life = 0;
        }
        any = any || p.life > 0;
    }
    if (shakeLeft > 0)
    {
        shakeLeft--;
        shakeX = shakeLeft > 0 ? (int8_t)rng.below(2 * SHAKE_PX + 1) - SHAKE_PX : 0;
        shakeY = shakeLeft > 0 ? (int8_t)rng.below(2 * SHAKE_PX + 1) - SHAKE_PX : 0;
        any = true;
    }
    busy = any;
}
