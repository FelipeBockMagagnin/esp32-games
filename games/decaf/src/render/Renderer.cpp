#include "Renderer.h"

static const uint16_t FLAG_POLE = 0x8C71;
static const uint16_t FLAG_OFF = 0x6B4D;
static const uint16_t FLAG_ON = 0xFE60;

// Office furniture behind the tiles, in screen rows. Windows scroll at a quarter of the camera,
// ceiling lights and cubicles at half, so the floor feels deep.
static const int16_t CEILING_H = 12;
static const int16_t LIGHT_TOP = 4;
static const int16_t LIGHT_BOTTOM = 7;
static const int16_t WINDOW_TOP = 28;
static const int16_t WINDOW_BOTTOM = 108;
static const int16_t WINDOW_BAR = 66;
static const int16_t MONITOR_TOP = 136;
static const int16_t PARTITION_TOP = 150;
static const int16_t PIT_TOP = 204;

static inline uint16_t swapped(uint16_t c)
{
    return (uint16_t)((c >> 8) | (c << 8));
}

static uint16_t blend(uint16_t a, uint16_t b, int16_t t, int16_t range)
{
    int16_t r = ((a >> 11) * (range - t) + (b >> 11) * t) / range;
    int16_t g = (((a >> 5) & 63) * (range - t) + ((b >> 5) & 63) * t) / range;
    int16_t bl = ((a & 31) * (range - t) + (b & 31) * t) / range;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

void Renderer::beginFrame(const Stage &stage)
{
    const Backdrop &b = stage.getDef().backdrop;
    for (int16_t y = 0; y < VIEW_H; y++)
    {
        wallRows[y] = swapped(blend(b.wallTop, b.wallBottom, y, VIEW_H));
    }
    camX = stage.getCameraX();
    drawCount = stage.collectDraws(draws, Stage::MAX_DRAWS);
}

void Renderer::renderBand(const Stage &stage, uint8_t band, uint16_t *out) const
{
    int16_t top = band * BAND_H;
    backdrop(stage.getDef().backdrop, top, out);
    tiles(stage, band, out);
    for (uint8_t i = 0; i < drawCount; i++)
    {
        if (draws[i].kind == SpriteDraw::FLAG)
        {
            flag(draws[i], top, out);
        }
        else
        {
            sprite(draws[i], top, out);
        }
    }
}

void Renderer::backdrop(const Backdrop &b, int16_t top, uint16_t *out) const
{
    const uint16_t ceiling = swapped(b.ceiling), light = swapped(b.light);
    const uint16_t window = swapped(b.window), glow = swapped(b.windowGlow), frame = swapped(b.windowFrame);
    const uint16_t partition = swapped(b.partition), cap = swapped(b.partitionCap);
    const uint16_t monitor = swapped(b.monitor), screen = swapped(b.monitorGlow), pit = swapped(b.pit);
    const int16_t far = camX / 4;
    const int16_t mid = camX / 2;

    for (int16_t y = top; y < top + BAND_H; y++)
    {
        uint16_t *row = out + (y - top) * VIEW_W;
        const uint16_t wall = wallRows[y];
        if (y < CEILING_H)
        {
            bool lit = y >= LIGHT_TOP && y < LIGHT_BOTTOM;
            for (int16_t x = 0; x < VIEW_W; x++)
            {
                int16_t m = (mid + x) & 127;
                row[x] = lit && m >= 40 && m < 88 ? light : ceiling;
            }
        }
        else if (y >= WINDOW_TOP && y < WINDOW_BOTTOM)
        {
            bool rail = y < WINDOW_TOP + 2 || y >= WINDOW_BOTTOM - 2 || (y >= WINDOW_BAR && y < WINDOW_BAR + 2);
            for (int16_t x = 0; x < VIEW_W; x++)
            {
                int16_t m = (far + x) & 127;
                if (m < 16 || m >= 112)
                {
                    row[x] = wall;
                }
                else if (rail || m < 18 || m >= 110 || ((m - 16) & 31) < 2)
                {
                    row[x] = frame;
                }
                else
                {
                    row[x] = ((y + m / 2) % 28) < 3 ? glow : window;
                }
            }
        }
        else if (y >= PIT_TOP)
        {
            for (int16_t x = 0; x < VIEW_W; x++)
            {
                row[x] = pit;
            }
        }
        else if (y >= MONITOR_TOP)
        {
            bool panel = y >= PARTITION_TOP;
            bool capRow = y < PARTITION_TOP + 3;
            bool screenRow = y >= MONITOR_TOP + 2 && y < PARTITION_TOP - 3;
            for (int16_t x = 0; x < VIEW_W; x++)
            {
                int16_t m = (mid + x) & 127;
                if (panel)
                {
                    row[x] = m >= 104 ? wall : (capRow || m == 52) ? cap : partition;
                }
                else if (m >= 30 && m < 50)
                {
                    row[x] = screenRow && m >= 32 && m < 48 ? screen : monitor;
                }
                else
                {
                    row[x] = wall;
                }
            }
        }
        else
        {
            for (int16_t x = 0; x < VIEW_W; x++)
            {
                row[x] = wall;
            }
        }
    }
}

void Renderer::tiles(const Stage &stage, uint8_t band, uint16_t *out) const
{
    int16_t first = camX / TILE;
    int16_t last = (camX + VIEW_W - 1) / TILE;
    for (int16_t col = first; col <= last; col++)
    {
        uint8_t art = stage.artAt(col, band);
        if (art >= TILE_ART_COUNT)
        {
            continue;
        }
        const uint16_t *src = tile_art[art];
        int16_t sx = col * TILE - camX;
        int16_t x0 = sx < 0 ? -sx : 0;
        int16_t x1 = sx + TILE > VIEW_W ? VIEW_W - sx : TILE;
        for (int16_t y = 0; y < TILE; y++)
        {
            uint16_t *row = out + y * VIEW_W + sx;
            const uint16_t *line = src + y * TILE;
            for (int16_t x = x0; x < x1; x++)
            {
                uint16_t c = pgm_read_word(line + x);
                if (c != 0)
                {
                    row[x] = c;
                }
            }
        }
    }
}

void Renderer::sprite(const SpriteDraw &d, int16_t top, uint16_t *out) const
{
    const SheetDef &sheet = *d.sheet;
    if (d.frame >= sheet.count)
    {
        return;
    }
    const FrameDef &f = sheet.frames[d.frame];
    if (f.pixels == nullptr)
    {
        return;
    }
    int16_t boxLeft = d.x - camX + (d.flip ? sheet.frameW - f.x - f.w : f.x);
    int16_t boxTop = d.y + f.y;
    int16_t y0 = boxTop > top ? boxTop : top;
    int16_t y1 = boxTop + f.h < top + BAND_H ? boxTop + f.h : top + BAND_H;
    if (y0 >= y1 || boxLeft >= VIEW_W || boxLeft + f.w <= 0)
    {
        return;
    }
    int16_t i0 = boxLeft < 0 ? -boxLeft : 0;
    int16_t i1 = boxLeft + f.w > VIEW_W ? VIEW_W - boxLeft : f.w;
    uint16_t tint = swapped(d.color);

    for (int16_t y = y0; y < y1; y++)
    {
        const uint8_t *src = f.pixels + (y - boxTop) * f.w;
        uint16_t *row = out + (y - top) * VIEW_W + boxLeft;
        for (int16_t i = i0; i < i1; i++)
        {
            uint8_t index = pgm_read_byte(src + (d.flip ? f.w - 1 - i : i));
            if (index == 0)
            {
                continue;
            }
            row[i] = d.kind == SpriteDraw::SILHOUETTE ? tint : pgm_read_word(sheet.palette + index - 1);
        }
    }
}

// A post-it on a pole: grey until reached
void Renderer::flag(const SpriteDraw &d, int16_t top, uint16_t *out) const
{
    static const int16_t POLE_H = 28;
    static const int16_t FLAG_W = 9;
    static const int16_t FLAG_H = 6;
    int16_t x = d.x - camX;
    uint16_t pole = swapped(FLAG_POLE);
    uint16_t cloth = swapped(d.frame ? FLAG_ON : FLAG_OFF);
    for (int16_t y = d.y - POLE_H; y < d.y; y++)
    {
        if (y < top || y >= top + BAND_H)
        {
            continue;
        }
        uint16_t *row = out + (y - top) * VIEW_W;
        bool base = y >= d.y - 2;
        for (int16_t px = x - (base ? 2 : 0); px <= x + (base ? 2 : 0); px++)
        {
            if (px >= 0 && px < VIEW_W)
            {
                row[px] = pole;
            }
        }
        if (y < d.y - POLE_H + FLAG_H)
        {
            for (int16_t px = x + 1; px <= x + FLAG_W; px++)
            {
                if (px >= 0 && px < VIEW_W)
                {
                    row[px] = cloth;
                }
            }
        }
    }
}
