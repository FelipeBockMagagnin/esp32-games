#include "Effects.h"
#include <math.h>
#include <string.h>
#include "../jack/Config.h"

static const uint16_t COIN_COLOR = 0x60FE; // Gold, byte-swapped
static const float COIN_TARGET_X = ARENA_PX - 20;
static const float COIN_TARGET_Y = -6;
static const int8_t SHAKE_PX = 3;

void Effects::reset()
{
    memset(particles, 0, sizeof(particles));
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

void Effects::burst(float x, float y, uint16_t colorA, uint16_t colorB, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        Particle *p = freeSlot();
        if (p == nullptr)
        {
            return;
        }
        float angle = rng.unit() * 6.2832f;
        float speed = 1.0f + rng.unit() * 2.5f;
        *p = {x, y, cosf(angle) * speed, sinf(angle) * speed, (i & 1) ? colorB : colorA,
              (uint8_t)(18 + rng.below(14)), (uint8_t)(2 + rng.below(3)), false};
    }
    busy = true;
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
        *p = {x, y, (rng.unit() - 0.5f) * 4.0f, (rng.unit() - 0.5f) * 4.0f, COIN_COLOR, 90, 3, true};
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
            if (d < 6)
            {
                p.life = 0;
                continue;
            }
            p.vx = p.vx * 0.85f + dx / d * 1.2f;
            p.vy = p.vy * 0.85f + dy / d * 1.2f;
        }
        else
        {
            p.vx *= 0.92f;
            p.vy *= 0.92f;
        }
        p.x += p.vx;
        p.y += p.vy;
        p.life--;
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
