#include "Particles.h"
#include <math.h>
#include "../gfx/Icons.h"
#include "../gfx/Theme.h"

void Particles::clear()
{
    for (auto &p : pool)
    {
        p.alive = false;
    }
}

void Particles::spawn(unsigned long now, Bit kind, float x, float y, float vx, float vy, uint16_t lifeMs, float size,
                      uint8_t color, float gravity, bool glow)
{
    // Past COUNT the oldest makes way
    Particle *slot = &pool[0];
    for (auto &p : pool)
    {
        if (!p.alive)
        {
            slot = &p;
            break;
        }
        if (p.born < slot->born)
        {
            slot = &p;
        }
    }
    *slot = {x, y, vx, vy, size, gravity, now, lifeMs, kind, color, glow, true};
}

void Particles::update(unsigned long now, float dt)
{
    for (auto &p : pool)
    {
        // One spawned for later waits its turn
        if (!p.alive || (long)(now - p.born) < 0)
        {
            continue;
        }
        if (now - p.born >= p.life)
        {
            p.alive = false;
            continue;
        }
        p.vy += p.gravity * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
    }
}

void Particles::draw(Canvas &c, unsigned long now, bool glow) const
{
    for (const auto &p : pool)
    {
        if (!p.alive || p.glow != glow || (long)(now - p.born) < 0 || !c.reaches(p.y - p.size, p.y + p.size))
        {
            continue;
        }
        float t = (float)(now - p.born) / p.life;
        // Pops in with a little overshoot, fades over its last third
        float pop = t < 0.12f ? 1.25f * t / 0.12f : t < 0.2f ? 1.25f - (t - 0.12f) / 0.08f * 0.25f : 1.0f;
        uint8_t alpha = t > 0.66f ? (uint8_t)((1 - t) / 0.34f * 255) : 255;
        float s = p.size * pop;
        uint16_t color = Theme::rgb(p.color), shine = Theme::rgb(SHINE);
        switch (p.kind)
        {
        case Bit::HEART:
            drawIcon(c, Icon::HEART, p.x, p.y, s, color, shine, alpha);
            break;
        case Bit::STAR:
            drawIcon(c, Icon::STAR, p.x, p.y, s, color, shine, alpha);
            break;
        case Bit::SPARKLE:
            drawIcon(c, Icon::SPARKLE, p.x, p.y, s, color, shine, alpha);
            break;
        case Bit::NOTE:
            drawIcon(c, Icon::NOTE, p.x, p.y, s, color, shine, alpha);
            break;
        case Bit::DROP:
            drawIcon(c, Icon::DROP, p.x, p.y, s, color, shine, alpha);
            break;
        case Bit::CROSS:
            drawIcon(c, Icon::CROSS, p.x, p.y, s, color, shine, alpha);
            break;
        case Bit::Z:
            c.text(p.x, p.y - s * 0.6f, s > 12 ? "Z" : "z", s > 12 ? FONT_TITLE : FONT_BODY, color, Canvas::CENTER,
                   alpha);
            break;
        case Bit::CONFETTI:
            c.fillRoundRect(p.x - s * 0.3f, p.y - s * 0.18f, s * 0.6f, s * 0.36f, s * 0.12f, color, alpha);
            break;
        case Bit::CRUMB:
            c.fillCircle(p.x, p.y, s * 0.25f, color, alpha);
            break;
        }
    }
}
