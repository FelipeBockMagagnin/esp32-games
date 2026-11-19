#ifndef FLUID_INK_SIM_H
#define FLUID_INK_SIM_H

#include <stdint.h>

struct Rgb
{
    float r, g, b;
};

// Stam's stable fluids on a W x H grid with a ring of wall cells round it, plus vorticity
// confinement to keep the swirls a coarse grid would smear away. Velocity is in cells per step,
// dye in 0..1 per channel. Only one projection per step, before advection, as the next step's
// projection cleans what advection leaves.
class InkSim
{
public:
    static const int W = 48;
    static const int H = 64;
    static const int STRIDE = W + 2;
    static const int CELLS = STRIDE * (H + 2);
    static const int PRESSURE_ITERATIONS = 20;

    ~InkSim() { end(); }

    // About 120 KB of heap; false when it is not there
    bool begin();
    void end();
    bool ready() const { return u != nullptr; }
    void clear();

    // A stroke at (x, y), in cells from the grid's top left corner (0..W, 0..H), moving at
    // (vx, vy): pushes the fluid that way and lays `ink` within about `radius` cells
    void stir(float x, float y, float vx, float vy, const Rgb &ink, float radius);
    void step();

    static int at(int i, int j) { return i + STRIDE * j; }
    float velocityU(int i, int j) const { return u[at(i, j)]; }
    float velocityV(int i, int j) const { return v[at(i, j)]; }
    // The dye channels, indexed with at(); interior cells are 1..W, 1..H
    const float *channel(int c) const { return dye[c]; }
    float divergence() const; // Mean |div u| over the interior, for the tests
    float totalInk() const;

private:
    float *u = nullptr, *v = nullptr, *u0 = nullptr, *v0 = nullptr;
    float *p = nullptr, *scratch = nullptr;
    float *dye[3] = {nullptr, nullptr, nullptr};

    enum Edge : uint8_t
    {
        COPY,
        FLIP_U, // Horizontal velocity: no flow through the side walls
        FLIP_V
    };

    void walls(float *f, Edge edge);
    void confine(float strength);
    void project();
    void fadeEdges(float *f);
    void advect(float *dst, const float *src, float fade, Edge edge);
};

#endif // FLUID_INK_SIM_H
