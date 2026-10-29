#include "FieldView.h"
#include "TileArt.h"

void FieldView::place(int16_t x, int16_t y, int16_t tile)
{
    ox = x;
    oy = y;
    s = tile;
    revealOn = false;
}

void FieldView::cellOrigin(Cell c, int16_t &px, int16_t &py) const
{
    px = ox + c.x * s;
    py = oy + c.y * s;
}

void FieldView::draw(Cell c)
{
    int16_t px, py;
    cellOrigin(c, px, py);
    uint8_t p = parity(c);
    if (field.isOpen(c))
    {
        if (field.isMine(c))
        {
            TileArt::mine(tft, px, py, s, p, c == field.hitCell(), false);
        }
        else
        {
            TileArt::open(tft, px, py, s, p, field.count(c));
        }
    }
    else if (field.isFlagged(c))
    {
        TileArt::flag(tft, px, py, s, p, TileArt::FLAG_FRAMES - 1);
    }
    else
    {
        TileArt::covered(tft, px, py, s, p);
    }
}

void FieldView::drawAll()
{
    revealOn = false;
    for (uint8_t y = 0; y < field.rows(); y++)
    {
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            draw({x, y});
        }
    }
}

void FieldView::beginReveal()
{
    revealOn = field.openedCount() > 0;
    revealStart = 0;
    revealT = 0;
}

// A cell of wave w shows sinking frame t - w, then opens for good once that passes POP_FRAMES
int16_t FieldView::revealStep()
{
    if (!revealOn)
    {
        return -1;
    }
    uint16_t n = field.openedCount();
    while (revealStart < n && revealT - field.opened(revealStart).wave > TileArt::POP_FRAMES)
    {
        revealStart++;
    }
    if (revealStart >= n)
    {
        revealOn = false;
        return -1;
    }
    bool started = false;
    for (uint16_t i = revealStart; i < n; i++)
    {
        const Field::Opened &o = field.opened(i);
        int16_t phase = revealT - o.wave;
        if (phase < 0)
        {
            break;
        }
        started |= phase == 0;
        if (phase >= TileArt::POP_FRAMES || field.isMine(o.cell))
        {
            draw(o.cell);
        }
        else
        {
            int16_t px, py;
            cellOrigin(o.cell, px, py);
            TileArt::popping(tft, px, py, s, parity(o.cell), (uint8_t)phase);
        }
    }
    int16_t t = revealT++;
    return started ? t : -1;
}

void FieldView::finishReveal()
{
    if (!revealOn)
    {
        return;
    }
    for (uint16_t i = revealStart; i < field.openedCount(); i++)
    {
        draw(field.opened(i).cell);
    }
    revealOn = false;
}

void FieldView::sweep(uint8_t d)
{
    for (uint8_t back = 0; back <= TileArt::FLAG_FRAMES; back++)
    {
        if (back > d)
        {
            break;
        }
        uint8_t k = d - back;
        for (uint8_t x = 0; x < field.cols(); x++)
        {
            if (k < x || k - x >= field.rows())
            {
                continue;
            }
            Cell c = {x, (uint8_t)(k - x)};
            int16_t px, py;
            cellOrigin(c, px, py);
            if (field.isMine(c) && back < TileArt::FLAG_FRAMES)
            {
                TileArt::flag(tft, px, py, s, parity(c), back);
            }
            else if (back == 0)
            {
                draw(c);
                TileArt::sparkle(tft, px, py, s);
            }
            else if (back == 1)
            {
                draw(c);
            }
        }
    }
}

void FieldView::cover(uint8_t d)
{
    for (uint8_t x = 0; x < field.cols(); x++)
    {
        if (d >= x && d - x < field.rows())
        {
            Cell c = {x, (uint8_t)(d - x)};
            int16_t px, py;
            cellOrigin(c, px, py);
            TileArt::covered(tft, px, py, s, parity(c));
        }
    }
}
