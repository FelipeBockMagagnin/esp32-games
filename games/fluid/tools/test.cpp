// Tests on the PC: the ink stays divergence-free and finite, the water settles, holds its volume
// and stays in the box through tilts and shakes, the renderer reads the settled water as one
// body, and the saved prefs. Build and run with tools/test.sh.
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/fluid/FieldRender.h"
#include "../src/fluid/InkSim.h"
#include "../src/fluid/Palettes.h"
#include "../src/fluid/Prefs.h"
#include "../src/fluid/WaterSim.h"
#include <Preferences.h>

long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static void testInk()
{
    InkSim ink;
    assert(ink.begin());
    float x = 24, y = 32;
    uint32_t s = 7;
    for (int f = 0; f < 1500; f++)
    {
        s = s * 1103515245u + 12345u;
        float vx = ((s >> 8) % 300) / 100.0f - 1.5f, vy = ((s >> 18) % 300) / 100.0f - 1.5f;
        x = fminf(fmaxf(x + vx, 1.5f), InkSim::W - 1.5f);
        y = fminf(fmaxf(y + vy, 1.5f), InkSim::H - 1.5f);
        ink.stir(x, y, vx * 1.6f, vy * 1.6f, inkAt(PALETTES[f % PALETTE_COUNT], f * 0.0025f), 2.6f);
        ink.step();
        assert(std::isfinite(ink.totalInk()));
    }
    float maxSpeed = 0;
    for (int j = 1; j <= InkSim::H; j++)
    {
        for (int i = 1; i <= InkSim::W; i++)
        {
            maxSpeed = fmaxf(maxSpeed, hypotf(ink.velocityU(i, j), ink.velocityV(i, j)));
            for (int c = 0; c < 3; c++)
            {
                float d = ink.channel(c)[InkSim::at(i, j)];
                assert(d >= -0.01f && d <= 1.01f);
            }
        }
    }
    printf("  ink: div %.4f, top speed %.2f cells/step, ink %.0f\n", ink.divergence(), maxSpeed, ink.totalInk());
    assert(ink.divergence() < 0.05f);
    assert(maxSpeed < 4.0f);
    float before = ink.totalInk();
    for (int f = 0; f < 600; f++)
    {
        ink.step();
    }
    assert(ink.totalInk() < before * 0.6f);
    ink.clear();
    assert(ink.totalInk() == 0);
    printf("  ink ok\n");
}

static void settle(WaterSim &w, int frames)
{
    for (int f = 0; f < frames * 2; f++)
    {
        w.step();
    }
}

static void inside(const WaterSim &w)
{
    for (int i = 0; i < w.count(); i++)
    {
        assert(std::isfinite(w.px(i)) && std::isfinite(w.py(i)));
        assert(w.px(i) >= 0 && w.px(i) <= WaterSim::W && w.py(i) >= 0 && w.py(i) <= WaterSim::H);
    }
}

static float meanX(const WaterSim &w)
{
    float s = 0;
    for (int i = 0; i < w.count(); i++)
    {
        s += w.px(i);
    }
    return s / w.count();
}

static float meanY(const WaterSim &w)
{
    float s = 0;
    for (int i = 0; i < w.count(); i++)
    {
        s += w.py(i);
    }
    return s / w.count();
}

static void testWater()
{
    WaterSim w;
    assert(w.begin());
    assert(w.count() == WaterSim::MAX_PARTICLES);
    settle(w, 400);
    inside(w);
    float top = WaterSim::H, maxDensity = 0, maxSpeed = 0;
    for (int i = 0; i < w.count(); i++)
    {
        top = fminf(top, w.py(i));
        maxDensity = fmaxf(maxDensity, w.density(i));
        maxSpeed = fmaxf(maxSpeed, w.speed(i));
    }
    printf("  water settled: surface at %.0f px, densest %.2f of rest, fastest %.2f px/step\n", top, maxDensity,
           maxSpeed);
    assert(top > WaterSim::H * 0.35f && top < WaterSim::H * 0.7f);
    assert(maxDensity < 1.4f);
    assert(maxSpeed < 1.0f);
    assert(meanY(w) > WaterSim::H * 0.7f);

    w.setGravity(-0.15f, 0);
    settle(w, 300);
    inside(w);
    assert(meanX(w) < WaterSim::W * 0.35f);
    w.setGravity(0, -0.15f);
    settle(w, 300);
    inside(w);
    assert(meanY(w) < WaterSim::H * 0.3f);

    for (int k = 0; k < 20; k++)
    {
        w.shake(0, 6, 1234u + k);
        settle(w, 10);
        inside(w);
    }
    w.setGravity(0, 0.15f);
    settle(w, 500);
    inside(w);
    top = WaterSim::H;
    for (int i = 0; i < w.count(); i++)
    {
        top = fminf(top, w.py(i));
    }
    assert(top > WaterSim::H * 0.35f);
    printf("  water ok\n");

    FieldRender render;
    render.setPalette(PALETTES[0]);
    assert(render.beginWater());
    render.splat(w);
    float body = render.waterLevel(FieldRender::WATER_W / 2, FieldRender::WATER_H - 6);
    float air = render.waterLevel(FieldRender::WATER_W / 2, 6);
    printf("  render: body %.1f, air %.1f (of 32 levels a unit)\n", body, air);
    assert(body > 20 && body < 48);
    assert(air == 0);
    std::vector<uint16_t> rows(FieldRender::FIELD_W * 16);
    render.waterRows(0, 16, rows.data());
    uint16_t bg = rgb565(PALETTES[0].background);
    assert(rows[0] == (uint16_t)(bg << 8 | bg >> 8));
    printf("  render ok\n");
}

static void testPrefs()
{
    SlotStore store("fluid");
    Prefs p;
    defaultPrefs(p);
    assert(!readPrefs(store, p));
    p.mode = (uint8_t)Mode::WATER;
    p.palette = 3;
    assert(writePrefs(store, p));
    Prefs back;
    defaultPrefs(back);
    assert(readPrefs(store, back));
    assert(back.mode == (uint8_t)Mode::WATER && back.palette == 3);
    p.palette = 200;
    assert(writePrefs(store, p));
    assert(readPrefs(store, back));
    assert(back.palette == 0);
    printf("  prefs ok\n");
}

int main()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    testInk();
    testWater();
    testPrefs();
    printf("All tests passed\n");
    return 0;
}
