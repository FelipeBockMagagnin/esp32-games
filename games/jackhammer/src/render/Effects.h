#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include "../jack/Rng.h"

// What a kill looks like besides the enemy going away: a burst in its colours, coins flying
// up to the counter (one per multiplier, as the Unity CoinParticles), and the camera shake.
// Positions are arena pixels; stepped at 60 Hz with the round.
class Effects
{
public:
    static const uint8_t MAX_PARTICLES = 96;

    struct Particle
    {
        float x;
        float y;
        float vx;
        float vy;
        uint16_t color; // Byte-swapped RGB565
        uint8_t life;   // Steps left; 0 is free
        uint8_t size;
        bool coin;
    };

    void reset();
    void burst(float x, float y, uint16_t colorA, uint16_t colorB, uint8_t count);
    void coins(float x, float y, uint8_t count);
    void shake(uint8_t steps);
    void step();

    bool isBusy() const { return busy; }
    int8_t getShakeX() const { return shakeX; }
    int8_t getShakeY() const { return shakeY; }
    const Particle &getParticle(uint8_t i) const { return particles[i]; }

private:
    Particle particles[MAX_PARTICLES];
    Rng rng{0x51A7};
    uint8_t shakeLeft = 0;
    int8_t shakeX = 0;
    int8_t shakeY = 0;
    bool busy = false;

    Particle *freeSlot();
};

#endif // EFFECTS_H
