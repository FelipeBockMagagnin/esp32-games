#include "View.h"
#include <math.h>
#include <string.h>

// 66 degrees across: the focal length in pixels, for both axes so a tile looks square
static const float PLANE = 0.65f;
static const float FOCAL = (View::W / 2) / PLANE;
static const float SHADE_PER_TILE = 1.35f;
static const float NEAREST = 0.15f;
static const uint8_t PORTAL_FRAME_MS_SHIFT = 8; // A frame every 256 ms

// The view's own art: the dark above and the floor of each wall set, in Config.h's order
struct Vault
{
    uint32_t ceiling;
    uint32_t floor;
};
static const Vault VAULTS[WALL_SET_COUNT] = {
    {0x1c1a24, 0x3e3833},
    {0x121a18, 0x283830},
    {0x1e1612, 0x40321f},
    {0x1c0c0e, 0x3c1c1a},
};
// What addSprite takes for a row besides the flash: the distance's shade, or fully lit
static const uint8_t BY_DISTANCE = 0;
static const uint8_t LIT = 0xFF;
static const uint16_t HURT_RED = 0x00B8; // 0xB800, byte-swapped
static const uint16_t SIGHT_DOT = 0x18C6; // 0xC618
static const int16_t BOSS_BAR_X = 60;
static const int16_t BOSS_BAR_W = View::W - 2 * BOSS_BAR_X;
static const int16_t BOSS_BAR_Y = 6;
static const int16_t BOSS_BAR_H = 6;
static const uint16_t BOSS_FILL = 0x00C0;  // 0xC000
static const uint16_t BOSS_EMPTY = 0x0430; // 0x3004
static const uint16_t BOSS_EDGE = 0xA652;  // 0x52A6

static uint16_t swap16(uint16_t c)
{
    return (uint16_t)(c >> 8 | c << 8);
}

static uint8_t shadeFor(float dist)
{
    float level = dist * SHADE_PER_TILE;
    return level >= SHADES - 1 ? SHADES - 1 : (uint8_t)level;
}

static uint16_t shadeRgb(uint32_t rgb, uint8_t level)
{
    float f = powf(0.82f, level);
    uint32_t r = (uint32_t)(((rgb >> 16) & 255) * f);
    uint32_t g = (uint32_t)(((rgb >> 8) & 255) * powf(f, 1.1f));
    uint32_t b = (uint32_t)((rgb & 255) * powf(f, 1.3f));
    return swap16((uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3)));
}

static const Bitmap &wallBitmap(uint8_t cell, const Run &run)
{
    switch (cell)
    {
    case CELL_STONE:
        return WALL_ART[ART_WALL_STONE];
    case CELL_SEWER:
        return WALL_ART[ART_WALL_SEWER];
    case CELL_BONE:
        return WALL_ART[ART_WALL_BONE];
    case CELL_BLOOD:
        return WALL_ART[ART_WALL_BLOOD];
    case CELL_PORTAL:
        if (run.isSealed())
        {
            return WALL_ART[ART_WALL_SEALED];
        }
        return WALL_ART[(run.getClock() >> PORTAL_FRAME_MS_SHIFT) & 1 ? ART_WALL_PORTAL_1 : ART_WALL_PORTAL_0];
    default:
        return WALL_ART[ART_WALL_CRYPT];
    }
}

void View::paintRows(uint8_t set)
{
    if (set == rowSet)
    {
        return;
    }
    rowSet = set;
    for (int16_t y = 0; y < H; y++)
    {
        bool above = y < HORIZON;
        float fromHorizon = above ? HORIZON - y - 0.5f : y - HORIZON + 0.5f;
        float dist = 0.5f * FOCAL / fromHorizon;
        rowColor[y] = shadeRgb(above ? VAULTS[set].ceiling : VAULTS[set].floor, shadeFor(dist));
    }
}

void View::castColumns(const Run &run)
{
    const Map &map = run.getMap();
    float px = run.getX(), py = run.getY();
    float dirX = cosf(run.getAngle()), dirY = sinf(run.getAngle());
    float planeX = -dirY * PLANE, planeY = dirX * PLANE;
    for (int16_t c = 0; c < COLS; c++)
    {
        float camera = 2.0f * (2 * c + 1) / W - 1.0f;
        float rayX = dirX + planeX * camera, rayY = dirY + planeY * camera;
        int16_t mx = (int16_t)px, my = (int16_t)py;
        float deltaX = rayX == 0 ? 1e30f : fabsf(1.0f / rayX);
        float deltaY = rayY == 0 ? 1e30f : fabsf(1.0f / rayY);
        int8_t stepX = rayX < 0 ? -1 : 1, stepY = rayY < 0 ? -1 : 1;
        float sideX = rayX < 0 ? (px - mx) * deltaX : (mx + 1.0f - px) * deltaX;
        float sideY = rayY < 0 ? (py - my) * deltaY : (my + 1.0f - py) * deltaY;
        bool ySide = false;
        uint8_t cell = CELL_FLOOR;
        for (uint8_t guard = 0; guard < MAP_W + MAP_H && cell == CELL_FLOOR; guard++)
        {
            if (sideX < sideY)
            {
                sideX += deltaX;
                mx += stepX;
                ySide = false;
            }
            else
            {
                sideY += deltaY;
                my += stepY;
                ySide = true;
            }
            cell = map.at(mx, my);
        }
        float dist = ySide ? sideY - deltaY : sideX - deltaX;
        dist = dist < NEAREST ? NEAREST : dist;
        float hit = ySide ? px + dist * rayX : py + dist * rayY;
        hit -= floorf(hit);

        Column &col = cols[c];
        col.dist = dist;
        col.tex = &wallBitmap(cell, run);
        int16_t texX = (int16_t)(hit * col.tex->w);
        // Mirrored on two faces, so a texture reads the same way round from every side
        if ((!ySide && rayX < 0) || (ySide && rayY > 0))
        {
            texX = col.tex->w - 1 - texX;
        }
        col.texX = (uint8_t)texX;
        float height = FOCAL / dist;
        int32_t lineH = height > 16000 ? 16000 : (int32_t)height;
        lineH = lineH < 1 ? 1 : lineH;
        col.top = (int16_t)(HORIZON - lineH / 2);
        col.bottom = (int16_t)(col.top + lineH);
        col.step = ((uint32_t)col.tex->h << 16) / (uint32_t)lineH;
        uint8_t shade = shadeFor(dist) + (ySide ? 1 : 0);
        col.row = shade >= SHADES ? SHADES - 1 : shade;
    }
}

void View::addSprite(const Run &run, const Bitmap &bmp, float x, float y, float size, float lift, uint8_t row)
{
    if (spriteCount >= MAX_SPRITES)
    {
        return;
    }
    float dx = x - run.getX(), dy = y - run.getY();
    float c = cosf(run.getAngle()), s = sinf(run.getAngle());
    float depth = dx * c + dy * s;
    if (depth < 0.2f)
    {
        return;
    }
    float side = -dx * s + dy * c;
    float scale = FOCAL / depth;
    int16_t height = (int16_t)(size * scale);
    int16_t width = (int16_t)(height * bmp.w / bmp.h);
    if (height < 2 || width < 2)
    {
        return;
    }
    int16_t centerX = (int16_t)(W / 2 + side / PLANE * (W / 2) / depth);
    int16_t floorY = (int16_t)(HORIZON + 0.5f * scale);
    Quad &q = sprites[spriteCount];
    q.bmp = &bmp;
    q.dist = depth;
    q.width = width;
    q.height = height;
    q.left = centerX - width / 2;
    q.top = (int16_t)(floorY - height - lift * scale);
    q.c0 = q.left < 0 ? 0 : q.left / 2;
    q.c1 = (q.left + width + 1) / 2;
    q.c1 = q.c1 > COLS ? COLS : q.c1;
    if (q.c0 >= q.c1 || q.top >= H || q.top + height <= 0)
    {
        return;
    }
    q.uStep = ((uint32_t)bmp.w << 16) / (uint32_t)width;
    q.vStep = ((uint32_t)bmp.h << 16) / (uint32_t)height;
    q.row = row == BY_DISTANCE ? shadeFor(depth) : row == LIT ? 0 : row;
    spriteCount++;
}

void View::addOverlay(const Bitmap &bmp, int16_t left, int16_t top, int16_t width, int16_t height, uint8_t row)
{
    if (overlayCount >= MAX_OVERLAYS)
    {
        return;
    }
    Quad &q = overlays[overlayCount++];
    q.bmp = &bmp;
    q.dist = 0;
    q.left = left;
    q.top = top;
    q.width = width;
    q.height = height;
    q.c0 = left < 0 ? 0 : left / 2;
    q.c1 = (left + width + 1) / 2;
    q.c1 = q.c1 > COLS ? COLS : q.c1;
    q.uStep = ((uint32_t)bmp.w << 16) / (uint32_t)width;
    q.vStep = ((uint32_t)bmp.h << 16) / (uint32_t)height;
    q.row = row;
}

void View::prepare(const Run &run)
{
    paintRows(wallSetFor(run.getFloor()));
    castColumns(run);
    hurt = run.getHurtMs() > 0;
    const Enemy *boss = run.getBoss();
    bossFill = -1;
    if (boss != nullptr && boss->state != EnemyState::IDLE)
    {
        bossFill = (int16_t)((int32_t)(BOSS_BAR_W - 2) * (boss->hp > 0 ? boss->hp : 0) / boss->maxHp);
    }

    float moved = fabsf(run.getX() - lastX) + fabsf(run.getY() - lastY);
    lastX = run.getX();
    lastY = run.getY();
    if (moved < 0.5f)
    {
        bobPhase += moved * 9.0f;
    }
    int16_t bob = (int16_t)(fabsf(sinf(bobPhase)) * 6);

    spriteCount = 0;
    uint32_t clock = run.getClock();
    for (uint8_t i = 0; i < Run::MAX_ENEMIES; i++)
    {
        const Enemy &e = run.getEnemies()[i];
        if (e.state == EnemyState::DEAD)
        {
            continue;
        }
        const EnemyDef &def = ENEMIES[e.kind];
        float size = def.scale;
        float lift = 0;
        if (e.state == EnemyState::DYING)
        {
            size *= 0.2f + 0.8f * e.timer / Run::DYING_MS;
        }
        else if (e.state == EnemyState::WINDUP)
        {
            size *= 1.07f;
        }
        if (e.kind == ENEMY_WRAITH)
        {
            lift = 0.08f + 0.05f * sinf(clock / 300.0f + i);
        }
        uint8_t row = e.flashMs > 0 ? SHADES : BY_DISTANCE;
        addSprite(run, ENEMY_ART[e.kind], e.x, e.y, size, lift, row);
    }
    for (uint8_t i = 0; i < Run::MAX_PICKUPS; i++)
    {
        const Pickup &p = run.getPickups()[i];
        if (!p.active)
        {
            continue;
        }
        uint8_t art;
        if (p.potion != 0)
        {
            art = ITEM_POTION_FIRST + p.potion - 1;
        }
        else
        {
            art = ITEM_SERIES_FIRST[p.item.slot] + p.item.series * ITEM_TIERS + p.item.tier;
        }
        float lift = 0.04f + 0.03f * sinf(clock / 250.0f + i);
        addSprite(run, ITEM_ART[art], p.x, p.y, 0.3f, lift, BY_DISTANCE);
    }
    for (uint8_t i = 0; i < Run::MAX_PROJECTILES; i++)
    {
        const Projectile &p = run.getProjectiles()[i];
        if (!p.active)
        {
            continue;
        }
        float size = p.hostile ? 0.22f : p.big ? 0.42f : 0.28f;
        addSprite(run, FX_ART[p.hostile ? ART_FX_BOLT : ART_FX_FIREBALL], p.x, p.y, size, 0.45f - size / 2, LIT);
    }
    for (uint8_t i = 0; i < Run::MAX_SPARKS; i++)
    {
        const Spark &s = run.getSparks()[i];
        if (s.timer > 0)
        {
            float size = 0.2f + 0.3f * s.timer / Run::SPARK_MS;
            addSprite(run, FX_ART[ART_FX_SPARK], s.x, s.y, size, 0.45f - size / 2, LIT);
        }
    }
    // Far to near, so the nearer one paints over
    for (uint8_t i = 1; i < spriteCount; i++)
    {
        Quad q = sprites[i];
        int8_t j = (int8_t)i - 1;
        while (j >= 0 && sprites[j].dist < q.dist)
        {
            sprites[j + 1] = sprites[j];
            j--;
        }
        sprites[j + 1] = q;
    }

    overlayCount = 0;
    uint16_t swing = run.getSwingMs();
    if (swing > SWING_MS * 2 / 5)
    {
        const Bitmap &b = HAND_ART[ART_HAND_SWORD_SWING];
        addOverlay(b, W - b.w * 2 - 52, H - b.h * 2 + 8, b.w * 2, b.h * 2, 1);
    }
    else
    {
        const Bitmap &b = HAND_ART[ART_HAND_SWORD_IDLE];
        // Drawn back up after a swing
        int16_t drop = swing * 40 / SWING_MS;
        addOverlay(b, W - b.w * 2 - 6, H - b.h * 2 + 16 + bob + drop, b.w * 2, b.h * 2, 1);
    }
    if (run.isCharging() || run.getCastMs() > 0)
    {
        bool charging = run.isCharging();
        const Bitmap &b = HAND_ART[charging ? ART_HAND_HAND_CHARGE : ART_HAND_HAND_CAST];
        int16_t left = 18, top = H - b.h * 2 + 18 + bob;
        addOverlay(b, left, top, b.w * 2, b.h * 2, 1);
        if (charging)
        {
            const Bitmap &orb = FX_ART[ART_FX_FIREBALL];
            int16_t size = 10 + run.getCharge() * 30 / 255;
            // The palm's middle in the hand's frame, doubled
            addOverlay(orb, left + 58 - size / 2, top + 40 - size, size, size, 0);
        }
    }
}

void View::drawQuad(const Quad &q, uint16_t *buffer, int16_t top, int16_t rows, bool depth) const
{
    int16_t y0 = q.top > top ? q.top : top;
    int16_t y1 = q.top + q.height < top + rows ? q.top + q.height : top + rows;
    if (y0 >= y1)
    {
        return;
    }
    const Bitmap &b = *q.bmp;
    const uint16_t *palette = b.shades + q.row * b.stride;
    uint32_t *out = (uint32_t *)buffer;
    for (int16_t c = q.c0; c < q.c1; c++)
    {
        if (depth && q.dist >= cols[c].dist)
        {
            continue;
        }
        int32_t sx = 2 * c + 1 - q.left;
        if (sx < 0)
        {
            continue;
        }
        uint32_t u = ((uint32_t)sx * q.uStep) >> 16;
        if (u >= b.w)
        {
            continue;
        }
        const uint8_t *texels = b.texels + u * b.h;
        uint32_t v = (uint32_t)(y0 - q.top) * q.vStep;
        uint32_t *pixel = out + (y0 - top) * (W / 2) + c;
        for (int16_t y = y0; y < y1; y++, v += q.vStep, pixel += W / 2)
        {
            uint8_t index = texels[v >> 16];
            if (index != 0)
            {
                uint32_t color = palette[index];
                *pixel = color | color << 16;
            }
        }
    }
}

void View::drawBand(uint16_t *buffer, int16_t top, int16_t rows) const
{
    uint32_t *out = (uint32_t *)buffer;
    int16_t end = top + rows;
    for (int16_t c = 0; c < COLS; c++)
    {
        const Column &col = cols[c];
        uint32_t *pixel = out + c;
        int16_t y = top;
        int16_t wallTop = col.top > top ? col.top : top;
        wallTop = wallTop > end ? end : wallTop;
        for (; y < wallTop; y++, pixel += W / 2)
        {
            *pixel = (uint32_t)rowColor[y] * 0x10001u;
        }
        int16_t wallEnd = col.bottom < end ? col.bottom : end;
        if (y < wallEnd)
        {
            const uint8_t *texels = col.tex->texels + col.texX * col.tex->h;
            const uint16_t *palette = col.tex->shades + col.row * col.tex->stride;
            uint32_t v = (uint32_t)(y - col.top) * col.step;
            for (; y < wallEnd; y++, v += col.step, pixel += W / 2)
            {
                uint32_t color = palette[texels[v >> 16]];
                *pixel = color | color << 16;
            }
        }
        for (; y < end; y++, pixel += W / 2)
        {
            *pixel = (uint32_t)rowColor[y] * 0x10001u;
        }
    }
    for (uint8_t i = 0; i < spriteCount; i++)
    {
        drawQuad(sprites[i], buffer, top, rows, true);
    }
    for (uint8_t i = 0; i < overlayCount; i++)
    {
        drawQuad(overlays[i], buffer, top, rows, false);
    }

    for (int16_t y = top; y < end; y++)
    {
        uint16_t *line = buffer + (y - top) * W;
        if (y >= HORIZON - 3 && y <= HORIZON + 3 && y != HORIZON)
        {
            line[W / 2] = SIGHT_DOT;
        }
        if (y == HORIZON)
        {
            line[W / 2 - 3] = line[W / 2 - 2] = line[W / 2 + 2] = line[W / 2 + 3] = SIGHT_DOT;
        }
        if (bossFill >= 0 && y >= BOSS_BAR_Y && y < BOSS_BAR_Y + BOSS_BAR_H)
        {
            bool edge = y == BOSS_BAR_Y || y == BOSS_BAR_Y + BOSS_BAR_H - 1;
            line[BOSS_BAR_X] = line[BOSS_BAR_X + BOSS_BAR_W - 1] = BOSS_EDGE;
            for (int16_t x = 1; x < BOSS_BAR_W - 1; x++)
            {
                line[BOSS_BAR_X + x] = edge ? BOSS_EDGE : x - 1 < bossFill ? BOSS_FILL : BOSS_EMPTY;
            }
        }
        if (hurt)
        {
            if (y < 4 || y >= H - 4)
            {
                for (int16_t x = 0; x < W; x++)
                {
                    line[x] = HURT_RED;
                }
            }
            else
            {
                line[0] = line[1] = line[2] = line[3] = HURT_RED;
                line[W - 1] = line[W - 2] = line[W - 3] = line[W - 4] = HURT_RED;
            }
        }
    }
}
