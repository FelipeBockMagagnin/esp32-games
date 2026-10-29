#ifndef ART_SPRING_H
#define ART_SPRING_H

#include <math.h>

// A damped spring toward `rest`. It integrates in steps of at most 4 ms however long the frame
// was: one step per frame diverges once frames pass ~100 ms, which a busy frame on the board does.
struct Spring
{
    float stiffness;
    float damping;
    float rest;
    float value;
    float velocity = 0;

    Spring(float stiffness, float damping, float rest = 0) : stiffness(stiffness), damping(damping), rest(rest), value(rest) {}

    void kick(float v) { velocity += v; }
    void reset()
    {
        value = rest;
        velocity = 0;
    }

    void step(float dt)
    {
        int n = (int)ceilf(dt / 0.004f);
        float h = n > 0 ? dt / n : 0;
        for (int i = 0; i < n; i++)
        {
            velocity += (-stiffness * (value - rest) - damping * velocity) * h;
            value += velocity * h;
        }
    }
};

#endif // ART_SPRING_H
