#ifndef FLUID_WATER_SIM_H
#define FLUID_WATER_SIM_H

#include <stdint.h>

// Water as particles in a closed W x H box, in pixels, by double density relaxation (Clavet,
// Beaudoin and Poulin, 2005): each particle is pushed off its neighbours by how crowded it is
// against a rest density, plus a near-density term that keeps them from clumping. Velocities
// come back out of the positions, so walls only need to clamp.
class WaterSim
{
public:
    static const int W = 240;
    static const int H = 320;
    static const int MAX_PARTICLES = 600;
    static constexpr float RADIUS = 20.0f; // Interaction radius
    static const int GRID_W = (int)(W / RADIUS) + 1;
    static const int GRID_H = (int)(H / RADIUS) + 1;

    ~WaterSim() { end(); }

    bool begin(); // About 16 KB of heap
    void end();
    bool ready() const { return x != nullptr; }

    // A block of still water along the bottom
    void fill(int count = MAX_PARTICLES);
    // Gravity pulls this way, in pixels per step squared
    void setGravity(float gx, float gy);
    // Every particle thrown by (dx, dy) pixels per step, a little scattered, as from a jolt
    void shake(float dx, float dy, uint32_t seed);
    void step();

    int count() const { return n; }
    float px(int i) const { return x[i]; }
    float py(int i) const { return y[i]; }
    float speed(int i) const;
    float density(int i) const; // Relative to rest, as of the last step; for the tests
    float gravityX() const { return gx; }
    float gravityY() const { return gy; }

private:
    float *x = nullptr, *y = nullptr, *vx = nullptr, *vy = nullptr, *ox = nullptr, *oy = nullptr;
    float *rho = nullptr;
    uint16_t *order = nullptr;
    uint16_t cellStart[GRID_W * GRID_H + 1];
    int n = 0;
    float gx = 0, gy = 0.3f;

    int cellOf(float px, float py) const;
    void sortIntoCells();
    void relax(float dt);
    void keepInside();
};

#endif // FLUID_WATER_SIM_H
