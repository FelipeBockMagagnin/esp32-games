#ifndef EFFECTS_H
#define EFFECTS_H

#include <stdint.h>
#include "StyleArt.h"
#include "../jack/Rng.h"

// The Unity particle systems (render/StyleArt.h), the coins flying up to the counter (one per
// multiplier, as the CoinParticles) and the camera shake. Positions are arena pixels; stepped
// at 60 Hz with the round. A particle flies straight, as Unity's did, until it leaves the arena
// or its life runs out.
class Effects
{
public:
    static const uint8_t MAX_PARTICLES = 200;

    struct Particle
    {
        float x;
        float y;
        float vx; // Pixels a step
        float vy;
        const ParticleTexture *texture;
        uint16_t tint; // RGB565
        uint16_t life; // Steps left; 0 is free
        uint8_t size;
        bool coin;
    };

    void reset();
    void emit(const Emitter &emitter, float x, float y, uint8_t count);
    // Keeps emitting from there at the emitter's rate and bursts; nullptr stops it
    void setSource(const Emitter *emitter, float x, float y);
    void coins(float x, float y, uint8_t count);
    void shake(uint8_t steps);
    void step();

    bool isBusy() const { return busy || source != nullptr; }
    int8_t getShakeX() const { return shakeX; }
    int8_t getShakeY() const { return shakeY; }
    const Particle &getParticle(uint8_t i) const { return particles[i]; }

private:
    Particle particles[MAX_PARTICLES];
    Rng rng{0x51A7};
    const Emitter *source = nullptr;
    float sourceX = 0;
    float sourceY = 0;
    float owed = 0;    // Particles the rate has built up, not thrown yet
    float burstIn = 0; // Seconds
    uint8_t shakeLeft = 0;
    int8_t shakeX = 0;
    int8_t shakeY = 0;
    bool busy = false;

    Particle *freeSlot();
    float between(float low, float high) { return low + (high - low) * rng.unit(); }
};

#endif // EFFECTS_H
