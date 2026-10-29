#include "ArenaRenderer.h"

static const int16_t CENTER = ARENA_PX / 2;
// Ticks per frame of the animated art (Pen and Watercolor), at 60 ticks a second
static const uint8_t ANIM_TICKS = 8;

static inline uint16_t swapped(uint16_t c)
{
    return (uint16_t)((c >> 8) | (c << 8));
}

void ArenaRenderer::renderBand(const Scene &scene, uint8_t band, uint16_t *out) const
{
    int16_t top = band * BAND_H;
    background(scene, top, out);

    const StyleArt &style = *scene.style;
    int16_t cx = CENTER + scene.effects->getShakeX();
    int16_t cy = CENTER + scene.effects->getShakeY();
    const Round &round = *scene.round;
    for (uint8_t i = 0; i < Round::MAX_ENEMIES; i++)
    {
        const Round::Enemy &e = round.getEnemy(i);
        if (!e.alive)
        {
            continue;
        }
        const Art &art = *style.enemies[e.look % style.enemyCount];
        int16_t d = (int16_t)(e.distance * PX_PER_UNIT + 0.5f);
        static const int8_t DX[SIDE_COUNT] = {0, 1, 0, -1};
        static const int8_t DY[SIDE_COUNT] = {-1, 0, 1, 0};
        uint8_t frame = (uint8_t)((scene.tick / ANIM_TICKS + e.look) % art.frames);
        blit(art, frame, cx + DX[e.side] * d, cy + DY[e.side] * d, 0, top, out);
    }

    if (scene.showPlayer)
    {
        const Art &art = *style.player;
        blit(art, (uint8_t)((scene.tick / ANIM_TICKS) % art.frames), cx, cy, round.getFacing(), top, out);
    }
    particles(*scene.effects, top, out);
}

void ArenaRenderer::background(const Scene &scene, int16_t top, uint16_t *out) const
{
    const Backdrop *backdrop = scene.style->backdrop;
    if (backdrop == nullptr)
    {
        uint16_t c = swapped(scene.background);
        for (int32_t i = 0; i < (int32_t)ARENA_PX * BAND_H; i++)
        {
            out[i] = c;
        }
        return;
    }
    for (int16_t y = 0; y < BAND_H; y++)
    {
        const uint8_t *src = backdrop->pixels + ((top + y) / 2) * backdrop->side;
        uint16_t *row = out + y * ARENA_PX;
        for (int16_t x = 0; x < ARENA_PX; x += 2)
        {
            uint16_t c = pgm_read_word(backdrop->palette + pgm_read_byte(src + x / 2));
            row[x] = c;
            row[x + 1] = c;
        }
    }
}

void ArenaRenderer::blit(const Art &art, uint8_t frame, int16_t x, int16_t y, uint8_t turns, int16_t top,
                         uint16_t *out) const
{
    // The art's box relative to its pivot, then turned: a pixel at (u, v) lands on (-v - 1, u)
    // after a clockwise quarter turn
    int16_t u0 = -art.pivotX, u1 = art.w - art.pivotX;
    int16_t v0 = -art.pivotY, v1 = art.h - art.pivotY;
    int16_t x0, x1, y0, y1;
    switch (turns & 3)
    {
    case 0:
        x0 = u0, x1 = u1, y0 = v0, y1 = v1;
        break;
    case 1:
        x0 = -v1, x1 = -v0, y0 = u0, y1 = u1;
        break;
    case 2:
        x0 = -u1, x1 = -u0, y0 = -v1, y1 = -v0;
        break;
    default:
        x0 = v0, x1 = v1, y0 = -u1, y1 = -u0;
        break;
    }
    int16_t rowFrom = y + y0 > top ? y + y0 : top;
    int16_t rowTo = y + y1 < top + BAND_H ? y + y1 : top + BAND_H;
    int16_t colFrom = x + x0 > 0 ? x + x0 : 0;
    int16_t colTo = x + x1 < ARENA_PX ? x + x1 : ARENA_PX;
    if (rowFrom >= rowTo || colFrom >= colTo)
    {
        return;
    }

    const uint8_t *pixels = art.pixels + (int32_t)frame * art.w * art.h;
    for (int16_t py = rowFrom; py < rowTo; py++)
    {
        int16_t dy = py - y;
        uint16_t *row = out + (py - top) * ARENA_PX;
        for (int16_t px = colFrom; px < colTo; px++)
        {
            int16_t dx = px - x, u, v;
            switch (turns & 3)
            {
            case 0:
                u = dx, v = dy;
                break;
            case 1:
                u = dy, v = -dx - 1;
                break;
            case 2:
                u = -dx - 1, v = -dy - 1;
                break;
            default:
                u = -dy - 1, v = dx;
                break;
            }
            uint8_t index = pgm_read_byte(pixels + (v + art.pivotY) * art.w + (u + art.pivotX));
            if (index != 0)
            {
                row[px] = pgm_read_word(art.palette + index - 1);
            }
        }
    }
}

void ArenaRenderer::particles(const Effects &effects, int16_t top, uint16_t *out) const
{
    int16_t sx = effects.getShakeX(), sy = effects.getShakeY();
    for (uint8_t i = 0; i < Effects::MAX_PARTICLES; i++)
    {
        const Effects::Particle &p = effects.getParticle(i);
        if (p.life == 0)
        {
            continue;
        }
        int16_t half = p.size / 2;
        int16_t left = (int16_t)p.x + sx - half, upper = (int16_t)p.y + sy - half;
        for (int16_t py = upper; py < upper + p.size; py++)
        {
            if (py < top || py >= top + BAND_H)
            {
                continue;
            }
            uint16_t *row = out + (py - top) * ARENA_PX;
            for (int16_t px = left; px < left + p.size; px++)
            {
                if (px >= 0 && px < ARENA_PX)
                {
                    row[px] = p.color;
                }
            }
        }
    }
}
