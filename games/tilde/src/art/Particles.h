#ifndef ART_PARTICLES_H
#define ART_PARTICLES_H

#include "../gfx/Canvas.h"

enum class Bit : uint8_t
{
    HEART,
    STAR,
    SPARKLE,
    NOTE,
    Z,
    DROP,
    CROSS,
    CONFETTI,
    CRUMB
};

// Hearts, notes, z's, sparkles and confetti: a fixed pool, each one popping in, drifting and
// fading out. Positions in pixels, speeds in pixels a second.
class Particles
{
public:
    static const uint8_t COUNT = 16;

    void clear();
    void spawn(unsigned long now, Bit kind, float x, float y, float vx, float vy, uint16_t lifeMs, float size,
               uint8_t color, float gravity = 0, bool glow = false);
    void update(unsigned long now, float dt);
    // `glow` ones are drawn after the room darkens, so they shine at night
    void draw(Canvas &c, unsigned long now, bool glow) const;

private:
    struct Particle
    {
        float x, y, vx, vy, size, gravity;
        unsigned long born;
        uint16_t life;
        Bit kind;
        uint8_t color;
        bool glow;
        bool alive;
    };
    Particle pool[COUNT] = {};
};

#endif // ART_PARTICLES_H
