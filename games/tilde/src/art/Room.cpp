#include "Room.h"
#include <math.h>
#include "../gfx/Icons.h"
#include "../gfx/Theme.h"

namespace Room
{
    static const int16_t WIN_X = 22, WIN_Y = 46, WIN_W = 76, WIN_H = 62;
    static const float R = 16;

    float fluffX(uint8_t spot)
    {
        return WALK_LEFT + spot * (WALK_RIGHT - WALK_LEFT) / (float)ROOM_SPOTS;
    }

    static float wave(unsigned long now, float periodMs, float phase = 0)
    {
        return sinf(now * 2 * (float)M_PI / periodMs + phase);
    }

    static void cloud(Canvas &c, float x, float y, float s, uint16_t color)
    {
        c.fillCircle(x, y, 5 * s, color, 230);
        c.fillCircle(x + 6 * s, y - 3 * s, 6 * s, color, 230);
        c.fillCircle(x + 12 * s, y, 5 * s, color, 230);
        c.fillRoundRect(x - 2 * s, y - 1 * s, 16 * s, 6 * s, 3 * s, color, 230);
    }

    static void window(Canvas &c, const State &s, unsigned long now)
    {
        if (!c.reaches(WIN_Y - 4, WIN_Y + WIN_H + 6))
        {
            return;
        }
        uint16_t frame = Theme::rgb(CARD);
        c.fillRoundRect(WIN_X - 4, WIN_Y - 4, WIN_W + 8, WIN_H + 8, 10, Canvas::mix(frame, Theme::rgb(SHADOW), 60));
        c.fillRoundRect(WIN_X - 3, WIN_Y - 3, WIN_W + 6, WIN_H + 6, 9, frame);
        uint16_t skyTop = Theme::rgb(s.stars ? SKY_NIGHT : SKY_DAY);
        uint16_t skyBottom = Canvas::mix(skyTop, Theme::rgb(SHINE), s.stars ? 40 : 110);
        c.fillRoundRectV(WIN_X, WIN_Y, WIN_W, WIN_H, 6, skyTop, skyBottom);
        c.setClip(WIN_X + 2, WIN_Y + 2, WIN_X + WIN_W - 2, WIN_Y + WIN_H - 2);
        if (s.stars)
        {
            uint16_t gold = Theme::rgb(GOLD);
            c.fillCircle(WIN_X + 56, WIN_Y + 16, 9, gold);
            c.fillCircle(WIN_X + 61, WIN_Y + 12, 8, skyTop);
            const float stars[4][2] = {{14, 12}, {30, 30}, {44, 8}, {16, 44}};
            for (int i = 0; i < 4; i++)
            {
                float tw = wave(now, 1800, i * 1.7f) * 0.5f + 0.5f;
                drawIcon(c, Icon::SPARKLE, WIN_X + stars[i][0], WIN_Y + stars[i][1], 5 + tw * 4, gold, gold,
                         (uint8_t)(120 + tw * 135));
            }
        }
        else
        {
            uint16_t sun = Theme::rgb(GOLD);
            float glow = wave(now, 3000) * 1.5f;
            c.fillCircle(WIN_X + 60, WIN_Y + 15, 12 + glow, sun, 70);
            c.fillCircle(WIN_X + 60, WIN_Y + 15, 8, sun);
            uint16_t white = Theme::rgb(SHINE);
            float drift = fmodf(now / 90.0f, WIN_W + 40) - 30;
            cloud(c, WIN_X + drift, WIN_Y + 32, 1.0f, white);
            cloud(c, WIN_X + fmodf(drift + 52, WIN_W + 40) - 20, WIN_Y + 48, 0.7f, white);
            // Now and then a bird crosses
            unsigned long cycle = now % 14000;
            if (cycle < 3000)
            {
                float bx = WIN_X + cycle * (WIN_W + 10) / 3000.0f - 5, by = WIN_Y + 22 + wave(now, 700) * 2;
                float flap = wave(now, 260) * 2;
                uint16_t ink = Theme::rgb(INK_SOFT);
                c.line(bx - 4, by - 1 - flap, bx, by + 1, 1.4f, ink);
                c.line(bx, by + 1, bx + 4, by - 1 - flap, 1.4f, ink);
            }
        }
        c.clearClip();
        c.fillRect(WIN_X + WIN_W / 2 - 1, WIN_Y, 3, WIN_H, frame);
        c.fillRect(WIN_X, WIN_Y + WIN_H / 2 - 1, WIN_W, 3, frame);
        // Curtains, drawn back
        uint16_t curtain = Theme::rgb(BLUSH);
        for (int side = 0; side < 2; side++)
        {
            float x = side == 0 ? WIN_X - 8 : WIN_X + WIN_W + 8, in = side == 0 ? 14 : -14;
            float swing = wave(now, 4200, side) * 1.5f;
            c.fillTriangle(x, WIN_Y - 6, x + in + swing, WIN_Y - 6, x + swing, WIN_Y + WIN_H + 4, curtain, 200);
        }
        c.fillRoundRect(WIN_X - 10, WIN_Y - 9, WIN_W + 20, 6, 3, Theme::rgb(FLOOR));
    }

    static void picture(Canvas &c)
    {
        if (!c.reaches(52, 84))
        {
            return;
        }
        float x = 122, y = 54;
        c.fillRoundRect(x, y, 30, 26, 5, Theme::rgb(FLOOR));
        c.fillRoundRect(x + 3, y + 3, 24, 20, 3, Theme::rgb(CARD));
        drawIcon(c, Icon::HEART, x + 15, y + 13, 14, Theme::rgb(BLUSH), Theme::rgb(SHINE));
    }

    static void lamp(Canvas &c, const Pet &pet, const State &s, unsigned long now)
    {
        uint16_t cord = Theme::rgb(INK_FAINT);
        c.line(LAMP_X, Y + 2, LAMP_X, Y + 30, 1.6f, cord);
        uint16_t shade = Theme::rgb(BODY_D);
        c.fillTriangle(LAMP_X - 16, Y + 46, LAMP_X + 16, Y + 46, LAMP_X, Y + 28, shade);
        c.fillRoundRect(LAMP_X - 17, Y + 42, 34, 7, 3.5f, shade);
        bool on = pet.isLightOn();
        // A short flicker when it comes on
        unsigned long since = now - s.lampAt;
        if (on && since < 450 && (since / 70) % 2 == 0)
        {
            on = false;
        }
        c.fillCircle(LAMP_X, Y + 51, 5, on ? Theme::rgb(GOLD) : Theme::rgb(INK_FAINT));
        if (on)
        {
            c.fillTriangle(LAMP_X, Y + 46, LAMP_X - 40, FLOOR_Y + 20, LAMP_X + 40, FLOOR_Y + 20, Theme::rgb(GOLD), 30);
        }
    }

    static void plant(Canvas &c, const State &s, unsigned long now)
    {
        if (!c.reaches(FLOOR_Y - 50, GROUND + 4))
        {
            return;
        }
        float poke = 0;
        unsigned long since = now - s.pokeAt;
        if (since < 1500)
        {
            poke = sinf(since / 90.0f) * (1 - since / 1500.0f) * 6;
        }
        float sway = wave(now, 3600) * 2 + poke;
        uint16_t leaf = Theme::rgb(LEAF), edge = Canvas::mix(leaf, Theme::rgb(EYE), 60);
        float bx = PLANT_X, by = GROUND - 22;
        const float leaves[5][3] = {{-10, -20, -1}, {9, -24, 1}, {-3, -36, 0}, {12, -12, 1}, {-13, -8, -1}};
        for (auto &l : leaves)
        {
            float lx = bx + l[0] + sway * (1 + l[1] / -20.0f), ly = by + l[1];
            c.line(bx, by, lx, ly, 2, edge);
            c.fillEllipse(lx, ly, 7, 4.5f, edge);
            c.fillEllipse(lx, ly - 0.5f, 5.8f, 3.4f, leaf);
        }
        uint16_t pot = Canvas::mix(Theme::rgb(ALERT), Theme::rgb(FLOOR), 120);
        c.fillEllipse(bx, GROUND + 1, 16, 3.5f, Theme::rgb(SHADOW), 140);
        c.fillTriangle(bx - 14, by, bx + 14, by, bx + 10, GROUND, pot);
        c.fillTriangle(bx - 14, by, bx + 10, GROUND, bx - 10, GROUND, pot);
        c.fillRoundRect(bx - 16, by - 3, 32, 7, 3.5f, Canvas::mix(pot, Theme::rgb(SHINE), 50));
    }

    static void fluff(Canvas &c, const Pet &pet, unsigned long now)
    {
        uint16_t grey = Theme::rgb(INK_FAINT), light = Canvas::mix(grey, Theme::rgb(SHINE), 90);
        for (uint8_t i = 0; i < pet.getFluff(); i++)
        {
            float x = fluffX(pet.getFluffSpot(i)), y = GROUND - 4 + wave(now, 2000, i) * 0.6f;
            c.fillCircle(x - 4, y, 4.5f, grey, 210);
            c.fillCircle(x + 3, y - 1, 5.5f, grey, 210);
            c.fillCircle(x, y - 4, 4, light, 210);
        }
    }

    void drawBack(Canvas &c, const Pet &pet, const State &s, unsigned long now)
    {
        if (!c.reaches(Y, Y + H))
        {
            return;
        }
        uint16_t wallTop = Theme::rgb(WALL_TOP), wallBottom = Theme::rgb(WALL_BOTTOM);
        uint16_t floor = Theme::rgb(FLOOR), joint = Theme::rgb(FLOOR_LINE);
        c.fillRoundRectV(X, Y, W, H, R, wallTop, wallBottom);
        // The floor is the card's bottom, cut square where it meets the wall
        c.fillRoundRectV(X, FLOOR_Y, W, Y + H - FLOOR_Y, R, floor, Canvas::mix(floor, joint, 80));
        c.fillRect(X, FLOOR_Y, W, (int16_t)R, floor);
        if (c.reaches(FLOOR_Y - 6, FLOOR_Y + 2))
        {
            c.fillRect(X, FLOOR_Y - 5, W, 5, Theme::rgb(CARD));
            c.fillRect(X, FLOOR_Y, W, 1, joint);
        }
        for (int16_t y = FLOOR_Y + 14; y < Y + H - 4; y += 14)
        {
            c.fillRect(X + 4, y, W - 8, 1, joint, 120);
        }
        // A beam of sun from the window, with dust drifting in it
        if (!s.stars)
        {
            uint16_t shine = Theme::rgb(SHINE);
            c.fillTriangle(WIN_X + 10, WIN_Y + WIN_H, WIN_X + WIN_W, WIN_Y + WIN_H, WIN_X + 60, GROUND, shine, 30);
            for (int i = 0; i < 6; i++)
            {
                float t = fmodf(now / 9000.0f + i / 6.0f, 1.0f);
                float dx = WIN_X + 20 + t * 80 + wave(now, 2500, i * 2) * 6, dy = WIN_Y + WIN_H + t * 60 + i * 4;
                c.fillCircle(dx, dy, 1.3f, shine, (uint8_t)(sinf(t * (float)M_PI) * 200));
            }
        }
        c.fillEllipse(118, GROUND + 2, 74, 11, Theme::rgb(RUG), 200);
        c.fillEllipse(118, GROUND + 2, 60, 7.5f, Canvas::mix(Theme::rgb(RUG), Theme::rgb(SHINE), 70), 160);

        window(c, s, now);
        picture(c);
        lamp(c, pet, s, now);
        plant(c, s, now);
        fluff(c, pet, now);
    }

    void drawNight(Canvas &c, const State &s, unsigned long now)
    {
        if (s.night <= 0 || !c.reaches(Y, Y + H))
        {
            return;
        }
        uint8_t a = (uint8_t)(s.night * 165);
        uint16_t tint = Theme::rgb(NIGHT_TINT);
        c.fillRoundRect(X, Y, W, H, R, tint, a);
        // The moon and stars shine through the dark
        if (s.stars && c.reaches(WIN_Y, WIN_Y + WIN_H))
        {
            uint16_t gold = Theme::rgb(GOLD);
            c.fillCircle(WIN_X + 56, WIN_Y + 16, 9, gold, a);
            c.fillCircle(WIN_X + 61, WIN_Y + 12, 8, Canvas::mix(Theme::rgb(SKY_NIGHT), Theme::rgb(NIGHT_TINT), a), a);
            float tw = wave(now, 1800) * 0.5f + 0.5f;
            drawIcon(c, Icon::SPARKLE, WIN_X + 44, WIN_Y + 8, 6 + tw * 3, gold, gold, a);
        }
    }
}
