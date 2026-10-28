// Films a floor with the game's own renderer, for reviewing a layout and drawing the site's
// mockup: `film map <level> <out.ppm>` lays the whole floor out in one strip, `film frame <level>
// <ticks> <out.ppm>` plays the solver's route (Solver.h) for that long and saves the 320x224 view.
#include <Arduino.h>
#include <random>
#include <vector>

#include "../../src/game/Stage.h"
#include "../../src/render/Renderer.h"
#include "Solver.h"

unsigned long millis() { return 0; }
long random(long max) { return max <= 0 ? 0 : std::rand() % max; }
long random(long min, long max) { return min + random(max - min); }

static void writePpm(const char *path, const std::vector<uint16_t> &pixels, int w, int h)
{
    FILE *f = fopen(path, "wb");
    if (f == nullptr)
    {
        perror(path);
        exit(1);
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (uint16_t c : pixels)
    {
        c = (uint16_t)((c >> 8) | (c << 8));
        uint8_t rgb[3] = {(uint8_t)((c >> 11) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63),
                          (uint8_t)((c & 31) * 255 / 31)};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

static void film(const Stage &stage, Renderer &renderer, int16_t camX, std::vector<uint16_t> &out, int outW, int at)
{
    static uint16_t band[VIEW_W * Renderer::BAND_H];
    renderer.beginFrame(stage);
    renderer.setCamera(camX);
    for (uint8_t b = 0; b < Renderer::BAND_COUNT; b++)
    {
        renderer.renderBand(stage, b, band);
        for (int y = 0; y < Renderer::BAND_H; y++)
        {
            for (int x = 0; x < VIEW_W && at + x < outW; x++)
            {
                out[(b * Renderer::BAND_H + y) * outW + at + x] = band[y * VIEW_W + x];
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "usage: film map <level> <out.ppm> | film frame <level> <ticks> <out.ppm>\n");
        return 2;
    }
    std::string mode = argv[1];
    uint8_t level = (uint8_t)atoi(argv[2]);
    static Stage stage;
    static Renderer renderer;
    stage.load(level);

    if (mode == "map")
    {
        int w = stage.getWidth() * TILE;
        std::vector<uint16_t> pixels((size_t)w * VIEW_H);
        for (int x = 0; x < w; x += VIEW_W)
        {
            film(stage, renderer, (int16_t)(x + VIEW_W > w ? w - VIEW_W : x), pixels, w, x + VIEW_W > w ? w - VIEW_W : x);
        }
        writePpm(argv[3], pixels, w, VIEW_H);
        return 0;
    }

    if (mode == "frame" && argc >= 5)
    {
        std::vector<Input> route = solve(level, false);
        long ticks = atol(argv[3]);
        for (long t = 0; t < ticks && t < (long)route.size(); t++)
        {
            stage.step(route[t]);
        }
        std::vector<uint16_t> pixels((size_t)VIEW_W * VIEW_H);
        renderer.beginFrame(stage);
        film(stage, renderer, stage.getCameraX(), pixels, VIEW_W, 0);
        writePpm(argv[4], pixels, VIEW_W, VIEW_H);
        return 0;
    }
    fprintf(stderr, "unknown mode %s\n", argv[1]);
    return 2;
}
