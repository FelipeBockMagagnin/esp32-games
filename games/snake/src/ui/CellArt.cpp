#include "CellArt.h"

namespace CellArt
{
    // A rect given facing right, turned to face `d`
    static void fill(TFT_eSPI &tft, int16_t px, int16_t py, Dir d, int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t color)
    {
        switch (d)
        {
        case Dir::RIGHT:
            tft.fillRect(px + x, py + y, w, h, color);
            break;
        case Dir::LEFT:
            tft.fillRect(px + SIZE - x - w, py + y, w, h, color);
            break;
        case Dir::DOWN:
            tft.fillRect(px + y, py + x, h, w, color);
            break;
        case Dir::UP:
            tft.fillRect(px + y, py + SIZE - x - w, h, w, color);
            break;
        }
    }

    void empty(TFT_eSPI &tft, int16_t px, int16_t py)
    {
        tft.fillRect(px, py, SIZE, SIZE, Theme::FIELD);
        tft.fillRect(px + 1, py + 1, SIZE - 2, SIZE - 2, Theme::GHOST);
    }

    void body(TFT_eSPI &tft, int16_t px, int16_t py, uint8_t joins, bool fat, bool scaled)
    {
        empty(tft, px, py);
        uint16_t ink = Theme::INK;
        if (fat)
        {
            tft.fillRect(px + 1, py, 8, SIZE, ink);
            tft.fillRect(px, py + 1, SIZE, 8, ink);
        }
        else
        {
            tft.fillRect(px + 2, py + 2, 6, 6, ink);
        }
        for (uint8_t d = 0; d < 4; d++)
        {
            if (joins & (1 << d))
            {
                fill(tft, px, py, (Dir)d, 8, 2, 2, 6, ink);
            }
        }
        if (fat || scaled)
        {
            tft.fillRect(px + 4, py + 4, 2, 2, Theme::FIELD);
        }
    }

    void tail(TFT_eSPI &tft, int16_t px, int16_t py, Dir toward)
    {
        empty(tft, px, py);
        uint16_t ink = Theme::INK;
        fill(tft, px, py, toward, 8, 2, 2, 6, ink);
        fill(tft, px, py, toward, 4, 3, 4, 4, ink);
        fill(tft, px, py, toward, 2, 4, 2, 2, ink);
    }

    void head(TFT_eSPI &tft, int16_t px, int16_t py, Dir facing, bool mouthOpen, bool eyesShut)
    {
        empty(tft, px, py);
        uint16_t ink = Theme::INK, field = Theme::FIELD;
        fill(tft, px, py, facing, 0, 2, 2, 6, ink);
        fill(tft, px, py, facing, 1, 1, 6, 8, ink);
        fill(tft, px, py, facing, 7, 2, 2, 6, ink);
        if (mouthOpen)
        {
            fill(tft, px, py, facing, 6, 4, 4, 2, field);
            fill(tft, px, py, facing, 8, 3, 2, 4, field);
        }
        if (eyesShut)
        {
            fill(tft, px, py, facing, 4, 2, 2, 1, field);
            fill(tft, px, py, facing, 4, 7, 2, 1, field);
        }
        else
        {
            fill(tft, px, py, facing, 4, 2, 2, 2, field);
            fill(tft, px, py, facing, 4, 6, 2, 2, field);
        }
    }

    void food(TFT_eSPI &tft, int16_t px, int16_t py, bool big)
    {
        empty(tft, px, py);
        uint16_t color = Theme::FOOD;
        if (big)
        {
            tft.fillRect(px + 3, py + 2, 4, 6, color);
            tft.fillRect(px + 2, py + 3, 6, 4, color);
        }
        else
        {
            tft.fillRect(px + 4, py + 3, 2, 4, color);
            tft.fillRect(px + 3, py + 4, 4, 2, color);
        }
    }
}
