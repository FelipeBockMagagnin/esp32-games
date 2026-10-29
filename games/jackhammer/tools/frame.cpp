// Renders the arena with the game's own renderer on the PC, to check the art without a board:
// every style's title with its start particles over every style mid-round, and the Creepy hammer
// (which carries its fog and light) at its four facings, in one PPM.
// Build and run with tools/frame.sh [seconds] [out.ppm].
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../src/render/ArenaRenderer.h"

unsigned long millis() { return 0; }
long random(long max) { return max > 0 ? rand() % max : 0; }
long random(long min, long max) { return min + random(max - min); }

static const int GAP = 4;

static void aimNearest(Round &round)
{
    int target = -1;
    for (uint8_t i = 0; i < Round::MAX_ENEMIES; i++)
    {
        const Round::Enemy &e = round.getEnemy(i);
        if (e.alive && e.distance < 3.0f && (target < 0 || e.distance < round.getEnemy(target).distance))
        {
            target = i;
        }
    }
    if (target >= 0 && round.getFacing() != round.getEnemy(target).side)
    {
        uint8_t diff = (round.getEnemy(target).side + SIDE_COUNT - round.getFacing()) % SIDE_COUNT;
        round.turn(diff == 3 ? -1 : 1);
    }
}

static void render(const ArenaRenderer::Scene &scene, std::vector<uint16_t> &canvas, int canvasW, int left, int top)
{
    ArenaRenderer renderer;
    static uint16_t band[ARENA_PX * ArenaRenderer::BAND_H];
    for (uint8_t b = 0; b < ArenaRenderer::BAND_COUNT; b++)
    {
        renderer.renderBand(scene, b, band);
        for (int y = 0; y < ArenaRenderer::BAND_H; y++)
        {
            for (int x = 0; x < ARENA_PX; x++)
            {
                uint16_t c = band[y * ARENA_PX + x];
                canvas[(top + b * ArenaRenderer::BAND_H + y) * canvasW + left + x] = (uint16_t)((c >> 8) | (c << 8));
            }
        }
    }
}

int main(int argc, char **argv)
{
    float seconds = argc > 1 ? (float)atof(argv[1]) : 20.0f;
    const char *out = argc > 2 ? argv[2] : "frame.ppm";

    const int columns = STYLE_COUNT > (int)SIDE_COUNT ? STYLE_COUNT : (int)SIDE_COUNT;
    const int canvasW = columns * (ARENA_PX + GAP);
    const int canvasH = 3 * (ARENA_PX + GAP);
    std::vector<uint16_t> canvas(canvasW * canvasH, 0x8410);

    for (uint8_t s = 0; s < STYLE_COUNT; s++)
    {
        const StyleArt &style = STYLE_ART[s];
        Round round;
        Effects effects;
        effects.reset();
        round.start(1);
        effects.setSource(style.start, ARENA_PX / 2, ARENA_PX / 2);
        for (int i = 0; i < 120; i++)
        {
            effects.step();
        }
        ArenaRenderer::Scene title = {&style, style.backgrounds[0], &round, &effects, 0, true};
        render(title, canvas, canvasW, s * (ARENA_PX + GAP), 0);

        effects.reset();
        round.start(11 + s);
        uint8_t tick = 0;
        while (round.getSeconds() < seconds && !round.isDead())
        {
            aimNearest(round);
            round.step();
            round.takeEvents();
            for (uint8_t i = 0; i < round.getMarkCount(); i++)
            {
                const Round::Mark &m = round.getMark(i);
                static const int8_t DX[SIDE_COUNT] = {0, 1, 0, -1};
                static const int8_t DY[SIDE_COUNT] = {-1, 0, 1, 0};
                float x = ARENA_PX / 2 + DX[m.side] * m.distance * PX_PER_UNIT;
                float y = ARENA_PX / 2 + DY[m.side] * m.distance * PX_PER_UNIT;
                effects.emit(*style.death, x, y, style.death->burst);
                effects.coins(x, y, 3);
            }
            round.clearMarks();
            effects.step();
            tick++;
        }
        ArenaRenderer::Scene play = {&style, style.backgrounds[s % style.backgroundCount], &round, &effects, tick,
                                     true};
        render(play, canvas, canvasW, s * (ARENA_PX + GAP), ARENA_PX + GAP);
    }

    for (uint8_t f = 0; f < SIDE_COUNT; f++)
    {
        Round round;
        Effects effects;
        effects.reset();
        round.start(1);
        for (uint8_t t = 0; t < f; t++)
        {
            round.turn(1);
        }
        const StyleArt &style = STYLE_ART[3];
        ArenaRenderer::Scene scene = {&style, style.backgrounds[0], &round, &effects, 0, true};
        render(scene, canvas, canvasW, f * (ARENA_PX + GAP), 2 * (ARENA_PX + GAP));
    }

    FILE *file = fopen(out, "wb");
    if (file == nullptr)
    {
        return 1;
    }
    fprintf(file, "P6\n%d %d\n255\n", canvasW, canvasH);
    for (uint16_t c : canvas)
    {
        uint8_t rgb[3] = {(uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 63) << 2), (uint8_t)((c & 31) << 3)};
        fwrite(rgb, 1, 3, file);
    }
    fclose(file);
    printf("Wrote %s\n", out);
    return 0;
}
