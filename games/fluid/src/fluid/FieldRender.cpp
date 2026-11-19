#include "FieldRender.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// A particle spreads over this many grid cells; at the water's rest spacing their sum in the
// middle of the body comes to about 1 after NORMAL
static const float SPLAT_RADIUS = 3.6f;
static const float NORMAL = 1.0f / 3.2f;
// Density levels per unit in the colour table; foam starts at FOAM_FROM pixels per step and is
// full FOAM_SPEED above it
static const float FOAM_FROM = 2.5f;
static const float LEVELS = 32.0f;
static const int LEVEL_COUNT = 64;
static const float FOAM_SPEED = 6.0f;
// Dye spreads thin as it flows; this keeps a wisp visible on the dark background
static const float INK_GAIN = 1.7f;

FieldRender::Axis FieldRender::inkX, FieldRender::inkY, FieldRender::waterX, FieldRender::waterY;
bool FieldRender::axesReady = false;

static inline uint16_t swapped(int r5, int g6, int b5)
{
    uint16_t c = (uint16_t)(r5 << 11 | g6 << 5 | b5);
    return (uint16_t)(c << 8 | c >> 8);
}

static inline float clamp01(float v)
{
    return v < 0 ? 0 : (v > 1 ? 1 : v);
}

static inline float smooth(float a, float b, float v)
{
    float t = clamp01((v - a) / (b - a));
    return t * t * (3 - 2 * t);
}

void FieldRender::makeAxis(Axis &axis, int pixels, int cells, int scale)
{
    for (int p = 0; p < pixels; p++)
    {
        float g = (p + 0.5f) / scale - 0.5f;
        if (g <= 0)
        {
            axis.first[p] = 0;
            axis.frac[p] = 0;
        }
        else if (g >= cells - 1)
        {
            axis.first[p] = (uint8_t)(cells - 2);
            axis.frac[p] = 1;
        }
        else
        {
            axis.first[p] = (uint8_t)g;
            axis.frac[p] = g - (int)g;
        }
    }
}

void FieldRender::prepareAxes()
{
    if (axesReady)
    {
        return;
    }
    makeAxis(inkX, FIELD_W, InkSim::W, FIELD_W / InkSim::W);
    makeAxis(inkY, FIELD_H, InkSim::H, FIELD_H / InkSim::H);
    makeAxis(waterX, FIELD_W, WATER_W, WATER_CELL);
    makeAxis(waterY, FIELD_H, WATER_H, WATER_CELL);
    axesReady = true;
}

void FieldRender::setPalette(const Palette &palette)
{
    prepareAxes();
    Rgb bg = toRgb(palette.background);
    bgR = bg.r;
    bgG = bg.g;
    bgB = bg.b;
    Rgb body = toRgb(palette.water), rim = toRgb(palette.rim), froth = toRgb(palette.foam);
    for (int d = 0; d < LEVEL_COUNT; d++)
    {
        float level = (d + 0.5f) / LEVELS;
        float cover = smooth(0.30f, 0.42f, level);
        float edge = 1.0f - smooth(0.42f, 0.70f, level);
        float deep = 1.0f - 0.3f * smooth(0.8f, 1.8f, level);
        for (int f = 0; f < 8; f++)
        {
            float frothy = f / 7.0f * 0.85f;
            float c[3];
            const float b[3] = {body.r * deep, body.g * deep, body.b * deep};
            const float r[3] = {rim.r, rim.g, rim.b};
            const float o[3] = {froth.r, froth.g, froth.b};
            const float back[3] = {bgR, bgG, bgB};
            for (int k = 0; k < 3; k++)
            {
                float water = b[k] + (r[k] - b[k]) * edge * 0.6f;
                water += (o[k] - water) * frothy;
                c[k] = back[k] + (water - back[k]) * cover;
            }
            waterLut[d * 8 + f] = swapped((int)(c[0] * 31.99f), (int)(c[1] * 63.99f), (int)(c[2] * 31.99f));
        }
    }
}

// A 2x2 ordered dither, so the dark ends of the ink's fades step less
static const float DITHER[4] = {0.125f, 0.625f, 0.875f, 0.375f};

void FieldRender::inkRows(const InkSim &sim, int top, int rows, uint16_t *out)
{
    const float *ch[3] = {sim.channel(0), sim.channel(1), sim.channel(2)};
    const float bg[3] = {bgR, bgG, bgB};
    for (int y = top; y < top + rows; y++)
    {
        int j = inkY.first[y] + 1;
        float fy = inkY.frac[y];
        for (int c = 0; c < 3; c++)
        {
            const float *a = ch[c] + InkSim::at(1, j), *b = a + InkSim::STRIDE;
            for (int i = 0; i < InkSim::W; i++)
            {
                line[c][i] = bg[c] + (a[i] + (b[i] - a[i]) * fy) * INK_GAIN;
            }
        }
        const float *dither = DITHER + (y & 1) * 2;
        for (int x = 0; x < FIELD_W; x++)
        {
            int i = inkX.first[x];
            float fx = inkX.frac[x], d = dither[x & 1];
            float r = line[0][i] + (line[0][i + 1] - line[0][i]) * fx;
            float g = line[1][i] + (line[1][i + 1] - line[1][i]) * fx;
            float b = line[2][i] + (line[2][i + 1] - line[2][i]) * fx;
            int r5 = (int)(clamp01(r) * 31.0f + d);
            int g6 = (int)(clamp01(g) * 63.0f + d);
            int b5 = (int)(clamp01(b) * 31.0f + d);
            *out++ = swapped(r5, g6, b5);
        }
    }
}

bool FieldRender::beginWater()
{
    if (density != nullptr)
    {
        return true;
    }
    density = (float *)malloc(WATER_W * WATER_H * sizeof(float));
    foam = (float *)malloc(WATER_W * WATER_H * sizeof(float));
    if (density == nullptr || foam == nullptr)
    {
        endWater();
        return false;
    }
    return true;
}

void FieldRender::endWater()
{
    free(density);
    free(foam);
    density = foam = nullptr;
}

void FieldRender::splat(const WaterSim &sim)
{
    memset(density, 0, WATER_W * WATER_H * sizeof(float));
    memset(foam, 0, WATER_W * WATER_H * sizeof(float));
    const float r2 = SPLAT_RADIUS * SPLAT_RADIUS, inv = 1.0f / r2;
    const int reach = (int)SPLAT_RADIUS;
    for (int p = 0; p < sim.count(); p++)
    {
        float gx = sim.px(p) / WATER_CELL - 0.5f, gy = sim.py(p) / WATER_CELL - 0.5f;
        float s = sim.speed(p);
        int ci = (int)(gx + 0.5f), cj = (int)(gy + 0.5f);
        for (int j = cj - reach; j <= cj + reach; j++)
        {
            if (j < 0 || j >= WATER_H)
            {
                continue;
            }
            float dy = j - gy;
            for (int i = ci - reach; i <= ci + reach; i++)
            {
                if (i < 0 || i >= WATER_W)
                {
                    continue;
                }
                float dx = i - gx, d2 = dx * dx + dy * dy;
                if (d2 >= r2)
                {
                    continue;
                }
                float w = 1.0f - d2 * inv;
                w *= w;
                density[i + j * WATER_W] += w;
                foam[i + j * WATER_W] += w * s;
            }
        }
    }
    for (int k = 0; k < WATER_W * WATER_H; k++)
    {
        foam[k] = density[k] > 0.05f ? (foam[k] / density[k] - FOAM_FROM) * (8.0f / FOAM_SPEED) : 0;
        density[k] *= NORMAL * LEVELS;
    }
}

void FieldRender::waterRows(int top, int rows, uint16_t *out)
{
    for (int y = top; y < top + rows; y++)
    {
        int j = waterY.first[y];
        float fy = waterY.frac[y];
        const float *da = density + j * WATER_W, *fa = foam + j * WATER_W;
        for (int i = 0; i < WATER_W; i++)
        {
            line[0][i] = da[i] + (da[i + WATER_W] - da[i]) * fy;
            line[1][i] = fa[i] + (fa[i + WATER_W] - fa[i]) * fy;
        }
        for (int x = 0; x < FIELD_W; x++)
        {
            int i = waterX.first[x];
            float fx = waterX.frac[x];
            int d = (int)(line[0][i] + (line[0][i + 1] - line[0][i]) * fx);
            int f = (int)(line[1][i] + (line[1][i + 1] - line[1][i]) * fx);
            d = d > LEVEL_COUNT - 1 ? LEVEL_COUNT - 1 : d;
            f = f < 0 ? 0 : (f > 7 ? 7 : f);
            *out++ = waterLut[d * 8 + f];
        }
    }
}
