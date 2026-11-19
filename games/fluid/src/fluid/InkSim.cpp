#include "InkSim.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static const float VORTICITY = 0.35f;
static const float VELOCITY_FADE = 0.996f;
static const float DYE_FADE = 0.9985f;
// The walls hold the flow still, so dye would pool along them in a bright frame
static const float WALL_FADE = 0.97f;
static const float SOR = 1.7f;

bool InkSim::begin()
{
    if (ready())
    {
        return true;
    }
    float **arrays[] = {&u, &v, &u0, &v0, &p, &scratch, &dye[0], &dye[1], &dye[2]};
    for (float **a : arrays)
    {
        *a = (float *)malloc(CELLS * sizeof(float));
        if (*a == nullptr)
        {
            end();
            return false;
        }
    }
    clear();
    return true;
}

void InkSim::end()
{
    float **arrays[] = {&u, &v, &u0, &v0, &p, &scratch, &dye[0], &dye[1], &dye[2]};
    for (float **a : arrays)
    {
        free(*a);
        *a = nullptr;
    }
}

void InkSim::clear()
{
    float *arrays[] = {u, v, u0, v0, p, scratch, dye[0], dye[1], dye[2]};
    for (float *a : arrays)
    {
        memset(a, 0, CELLS * sizeof(float));
    }
}

void InkSim::stir(float x, float y, float vx, float vy, const Rgb &ink, float radius)
{
    const float colour[3] = {ink.r, ink.g, ink.b};
    float reach = radius * 2.0f;
    int i0 = (int)(x - reach), i1 = (int)(x + reach) + 1;
    int j0 = (int)(y - reach), j1 = (int)(y + reach) + 1;
    i0 = i0 < 1 ? 1 : i0;
    j0 = j0 < 1 ? 1 : j0;
    i1 = i1 > W ? W : i1;
    j1 = j1 > H ? H : j1;
    float inv = 1.0f / (radius * radius);
    for (int j = j0; j <= j1; j++)
    {
        for (int i = i0; i <= i1; i++)
        {
            float dx = (i - 0.5f) - x, dy = (j - 0.5f) - y;
            float w = expf(-(dx * dx + dy * dy) * inv);
            if (w < 0.02f)
            {
                continue;
            }
            int k = at(i, j);
            u[k] += (vx - u[k]) * w * 0.8f;
            v[k] += (vy - v[k]) * w * 0.8f;
            for (int c = 0; c < 3; c++)
            {
                dye[c][k] += (colour[c] - dye[c][k]) * w * 0.6f;
            }
        }
    }
}

void InkSim::walls(float *f, Edge edge)
{
    float sx = edge == FLIP_U ? -1.0f : 1.0f;
    float sy = edge == FLIP_V ? -1.0f : 1.0f;
    for (int j = 1; j <= H; j++)
    {
        f[at(0, j)] = sx * f[at(1, j)];
        f[at(W + 1, j)] = sx * f[at(W, j)];
    }
    for (int i = 1; i <= W; i++)
    {
        f[at(i, 0)] = sy * f[at(i, 1)];
        f[at(i, H + 1)] = sy * f[at(i, H)];
    }
    f[at(0, 0)] = 0.5f * (f[at(1, 0)] + f[at(0, 1)]);
    f[at(W + 1, 0)] = 0.5f * (f[at(W, 0)] + f[at(W + 1, 1)]);
    f[at(0, H + 1)] = 0.5f * (f[at(1, H + 1)] + f[at(0, H)]);
    f[at(W + 1, H + 1)] = 0.5f * (f[at(W, H + 1)] + f[at(W + 1, H)]);
}

// Pushes each swirl's edge round its centre, at the strength the curl's slope gives
void InkSim::confine(float strength)
{
    float *curl = scratch;
    for (int j = 1; j <= H; j++)
    {
        for (int i = 1; i <= W; i++)
        {
            int k = at(i, j);
            curl[k] = 0.5f * ((v[k + 1] - v[k - 1]) - (u[k + STRIDE] - u[k - STRIDE]));
        }
    }
    walls(curl, COPY);
    for (int j = 2; j < H; j++)
    {
        for (int i = 2; i < W; i++)
        {
            int k = at(i, j);
            float nx = 0.5f * (fabsf(curl[k + 1]) - fabsf(curl[k - 1]));
            float ny = 0.5f * (fabsf(curl[k + STRIDE]) - fabsf(curl[k - STRIDE]));
            float len = sqrtf(nx * nx + ny * ny) + 1e-5f;
            float w = curl[k] * strength / len;
            u[k] += ny * w;
            v[k] -= nx * w;
        }
    }
}

void InkSim::project()
{
    float *div = scratch;
    for (int j = 1; j <= H; j++)
    {
        for (int i = 1; i <= W; i++)
        {
            int k = at(i, j);
            div[k] = 0.5f * (u[k + 1] - u[k - 1] + v[k + STRIDE] - v[k - STRIDE]);
        }
    }
    // Warm-started from the last step's pressure, which is most of the way there already
    for (int n = 0; n < PRESSURE_ITERATIONS; n++)
    {
        for (int j = 1; j <= H; j++)
        {
            int k = at(1, j);
            for (int i = 1; i <= W; i++, k++)
            {
                float target = 0.25f * (p[k - 1] + p[k + 1] + p[k - STRIDE] + p[k + STRIDE] - div[k]);
                p[k] += SOR * (target - p[k]);
            }
        }
        walls(p, COPY);
    }
    for (int j = 1; j <= H; j++)
    {
        int k = at(1, j);
        for (int i = 1; i <= W; i++, k++)
        {
            u[k] -= 0.5f * (p[k + 1] - p[k - 1]);
            v[k] -= 0.5f * (p[k + STRIDE] - p[k - STRIDE]);
        }
    }
    walls(u, FLIP_U);
    walls(v, FLIP_V);
}

// Semi-Lagrangian: each cell takes what sat where its flow came from a step ago
void InkSim::advect(float *dst, const float *src, float fade, Edge edge)
{
    for (int j = 1; j <= H; j++)
    {
        int k = at(1, j);
        for (int i = 1; i <= W; i++, k++)
        {
            float x = i - u[k], y = j - v[k];
            x = x < 0.5f ? 0.5f : (x > W + 0.5f ? W + 0.5f : x);
            y = y < 0.5f ? 0.5f : (y > H + 0.5f ? H + 0.5f : y);
            int xi = (int)x, yi = (int)y;
            float fx = x - xi, fy = y - yi;
            const float *s = src + at(xi, yi);
            float top = s[0] + (s[1] - s[0]) * fx;
            float bottom = s[STRIDE] + (s[STRIDE + 1] - s[STRIDE]) * fx;
            dst[k] = (top + (bottom - top) * fy) * fade;
        }
    }
    walls(dst, edge);
}

void InkSim::step()
{
    confine(VORTICITY);
    project();
    advect(u0, u, VELOCITY_FADE, FLIP_U);
    advect(v0, v, VELOCITY_FADE, FLIP_V);
    for (int c = 0; c < 3; c++)
    {
        advect(scratch, dye[c], DYE_FADE, COPY);
        fadeEdges(scratch);
        float *t = dye[c];
        dye[c] = scratch;
        scratch = t;
    }
    float *t = u;
    u = u0;
    u0 = t;
    t = v;
    v = v0;
    v0 = t;
}

void InkSim::fadeEdges(float *f)
{
    for (int j = 1; j <= H; j++)
    {
        f[at(1, j)] *= WALL_FADE;
        f[at(W, j)] *= WALL_FADE;
    }
    for (int i = 2; i < W; i++)
    {
        f[at(i, 1)] *= WALL_FADE;
        f[at(i, H)] *= WALL_FADE;
    }
}

float InkSim::divergence() const
{
    float sum = 0;
    for (int j = 1; j <= H; j++)
    {
        for (int i = 1; i <= W; i++)
        {
            int k = at(i, j);
            sum += fabsf(0.5f * (u[k + 1] - u[k - 1] + v[k + STRIDE] - v[k - STRIDE]));
        }
    }
    return sum / (W * H);
}

float InkSim::totalInk() const
{
    float sum = 0;
    for (int j = 1; j <= H; j++)
    {
        for (int i = 1; i <= W; i++)
        {
            int k = at(i, j);
            sum += dye[0][k] + dye[1][k] + dye[2][k];
        }
    }
    return sum;
}
