#include "ArenaRenderer.h"

static const int16_t CENTER = ARENA_PX / 2;
// Ticks per frame of the animated art (Pen and Watercolor), at 60 ticks a second
static const uint8_t ANIM_TICKS = 8;
static const int16_t PROMPT_ABOVE = 52;

static inline uint16_t swapped(uint16_t c)
{
    return (uint16_t)((c >> 8) | (c << 8));
}

// A pixel at (u, v) from the pivot lands on (-v - 1, u) after a clockwise quarter turn
struct ArenaRenderer::Span
{
    int16_t rowFrom, rowTo, colFrom, colTo;

    Span(int16_t w, int16_t h, int16_t pivotX, int16_t pivotY, int16_t x, int16_t y, uint8_t turns, int16_t top)
    {
        int16_t u0 = -pivotX, u1 = w - pivotX, v0 = -pivotY, v1 = h - pivotY;
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
        rowFrom = y + y0 > top ? y + y0 : top;
        rowTo = y + y1 < top + BAND_H ? y + y1 : top + BAND_H;
        colFrom = x + x0 > 0 ? x + x0 : 0;
        colTo = x + x1 < ARENA_PX ? x + x1 : ARENA_PX;
    }

    bool empty() const { return rowFrom >= rowTo || colFrom >= colTo; }

    // The source pixel, from the pivot, behind a screen pixel at (dx, dy) from it
    static void source(int16_t dx, int16_t dy, uint8_t turns, int16_t &u, int16_t &v)
    {
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
    }
};

void ArenaRenderer::renderBand(const Scene &scene, uint8_t band, uint16_t *out) const
{
    int16_t top = band * BAND_H;
    background(scene, top, out);

    const StyleArt &style = *scene.style;
    int16_t cx = CENTER + scene.effects->getShakeX();
    int16_t cy = CENTER + scene.effects->getShakeY();
    const Round &round = *scene.round;
    if (scene.showPlayer)
    {
        for (uint8_t i = 0; i < style.glowCount; i++)
        {
            glow(*style.glows[i], cx, cy, round.getFacing(), top, out);
        }
    }
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
    particles(*scene.effects, false, top, out);

    if (scene.showPlayer)
    {
        const Art &art = *style.player;
        blit(art, (uint8_t)((scene.tick / ANIM_TICKS) % art.frames), cx, cy, round.getFacing(), top, out);
    }
    particles(*scene.effects, true, top, out);
    if (scene.prompt > 0)
    {
        prompt(cx, cy - PROMPT_ABOVE, scene.prompt, scene.promptColor, top, out);
    }
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
    Span span(art.w, art.h, art.pivotX, art.pivotY, x, y, turns, top);
    if (span.empty())
    {
        return;
    }
    const uint8_t *pixels = art.pixels + (int32_t)frame * art.w * art.h;
    for (int16_t py = span.rowFrom; py < span.rowTo; py++)
    {
        uint16_t *row = out + (py - top) * ARENA_PX;
        for (int16_t px = span.colFrom; px < span.colTo; px++)
        {
            int16_t u, v;
            Span::source(px - x, py - y, turns, u, v);
            uint8_t index = pgm_read_byte(pixels + (v + art.pivotY) * art.w + (u + art.pivotX));
            if (index != 0)
            {
                row[px] = pgm_read_word(art.palette + index - 1);
            }
        }
    }
}

void ArenaRenderer::glow(const Glow &g, int16_t x, int16_t y, uint8_t turns, int16_t top, uint16_t *out) const
{
    Span span(g.w, g.h, g.pivotX, g.pivotY, x, y, turns, top);
    if (span.empty())
    {
        return;
    }
    for (int16_t py = span.rowFrom; py < span.rowTo; py++)
    {
        uint16_t *row = out + (py - top) * ARENA_PX;
        for (int16_t px = span.colFrom; px < span.colTo; px++)
        {
            int16_t u, v;
            Span::source(px - x, py - y, turns, u, v);
            uint16_t a = pgm_read_byte(g.alpha + (int32_t)(v + g.pivotY) * g.w + (u + g.pivotX));
            if (a == 0)
            {
                continue;
            }
            uint16_t c = swapped(row[px]);
            uint16_t r = c >> 11, gr = (c >> 5) & 63, b = c & 31;
            r += ((31 - r) * a) >> 8;
            gr += ((63 - gr) * a) >> 8;
            b += ((31 - b) * a) >> 8;
            row[px] = swapped((uint16_t)((r << 11) | (gr << 5) | b));
        }
    }
}

static uint16_t multiply(uint16_t a, uint16_t b)
{
    uint16_t r = (a >> 11) * (b >> 11) / 31, g = ((a >> 5) & 63) * ((b >> 5) & 63) / 63, bl = (a & 31) * (b & 31) / 31;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

static uint16_t gradientAt(const ColorKey *keys, uint8_t count, float t)
{
    if (t <= keys[0].at)
    {
        return keys[0].color;
    }
    for (uint8_t i = 1; i < count; i++)
    {
        if (t <= keys[i].at)
        {
            float f = (t - keys[i - 1].at) / (keys[i].at - keys[i - 1].at);
            uint16_t a = keys[i - 1].color, b = keys[i].color;
            int r = (a >> 11) + (int)(((b >> 11) - (a >> 11)) * f);
            int g = ((a >> 5) & 63) + (int)((((b >> 5) & 63) - ((a >> 5) & 63)) * f);
            int bl = (a & 31) + (int)(((b & 31) - (a & 31)) * f);
            return (uint16_t)((r << 11) | (g << 5) | bl);
        }
    }
    return keys[count - 1].color;
}

static uint16_t blend(uint16_t dst, uint16_t src, uint16_t a, Blend mode)
{
    int dr = dst >> 11, dg = (dst >> 5) & 63, db = dst & 31;
    int sr = src >> 11, sg = (src >> 5) & 63, sb = src & 31;
    if (mode == Blend::ADD)
    {
        dr += sr * a >> 8;
        dg += sg * a >> 8;
        db += sb * a >> 8;
        dr = dr < 31 ? dr : 31;
        dg = dg < 63 ? dg : 63;
        db = db < 31 ? db : 31;
    }
    else
    {
        dr += (sr - dr) * a >> 8;
        dg += (sg - dg) * a >> 8;
        db += (sb - db) * a >> 8;
    }
    return (uint16_t)((dr << 11) | (dg << 5) | db);
}

void ArenaRenderer::particles(const Effects &effects, bool coins, int16_t top, uint16_t *out) const
{
    int16_t sx = effects.getShakeX(), sy = effects.getShakeY();
    for (uint8_t i = 0; i < Effects::MAX_PARTICLES; i++)
    {
        const Effects::Particle &p = effects.getParticle(i);
        if (p.life == 0 || p.coin != coins)
        {
            continue;
        }
        const Emitter *e = p.emitter;
        const ParticleTexture &t = e != nullptr ? *e->texture : PARTICLE_SQUARE;
        Blend mode = e != nullptr ? e->blend : Blend::OPAQUE;
        float age = 1.0f - (float)p.life / p.lifetime;
        float size = p.size;
        uint16_t color = p.tint;
        if (e != nullptr)
        {
            if (age > e->shrinkFrom)
            {
                size *= 1 + (e->sizeEnd - 1) * (age - e->shrinkFrom) / (1 - e->shrinkFrom);
            }
            if (e->gradient != nullptr)
            {
                color = multiply(color, gradientAt(e->gradient, e->gradientCount, age));
            }
        }
        // The size is the texture's long side, as Unity sized a particle's quad
        int16_t longSide = (int16_t)(size + 0.5f);
        if (longSide < 1)
        {
            continue;
        }
        int16_t w = t.w >= t.h ? longSide : (int16_t)(longSide * t.w / t.h);
        int16_t h = t.h >= t.w ? longSide : (int16_t)(longSide * t.h / t.w);
        w = w > 0 ? w : 1;
        h = h > 0 ? h : 1;
        int16_t left = (int16_t)p.x + sx - w / 2, upper = (int16_t)p.y + sy - h / 2;
        int16_t rowFrom = upper > top ? upper : top;
        int16_t rowTo = upper + h < top + BAND_H ? upper + h : top + BAND_H;
        if (rowFrom >= rowTo)
        {
            continue;
        }
        for (int16_t py = rowFrom; py < rowTo; py++)
        {
            uint16_t *row = out + (py - top) * ARENA_PX;
            int32_t srcRow = ((py - upper) * t.h / h) * t.w;
            for (int16_t px = left; px < left + w; px++)
            {
                if (px < 0 || px >= ARENA_PX)
                {
                    continue;
                }
                int32_t texel = srcRow + (px - left) * t.w / w;
                uint16_t a = pgm_read_byte(t.alpha + texel);
                if (mode == Blend::OPAQUE ? a < 128 : a == 0)
                {
                    continue;
                }
                uint16_t c = multiply(swapped(pgm_read_word(t.pixels + texel)), color);
                row[px] = swapped(mode == Blend::OPAQUE ? c : blend(swapped(row[px]), c, a + 1, mode));
            }
        }
    }
}

void ArenaRenderer::prompt(int16_t x, int16_t y, uint8_t radius, uint16_t color, int16_t top, uint16_t *out) const
{
    uint16_t fill = swapped(color);
    uint16_t rim = swapped(0xFFFF);
    int16_t r = radius, inner = (r - 2) * (r - 2), outer = r * r;
    for (int16_t py = y - r; py <= y + r; py++)
    {
        if (py < top || py >= top + BAND_H)
        {
            continue;
        }
        uint16_t *row = out + (py - top) * ARENA_PX;
        for (int16_t px = x - r; px <= x + r; px++)
        {
            int16_t d = (px - x) * (px - x) + (py - y) * (py - y);
            if (px >= 0 && px < ARENA_PX && d <= outer)
            {
                row[px] = d <= inner ? fill : rim;
            }
        }
    }
}
