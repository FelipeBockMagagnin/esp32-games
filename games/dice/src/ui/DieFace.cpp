#include "DieFace.h"

static const int16_t C = DieFace::SIZE / 2;
static const int16_t R = DieFace::SIZE / 2 - 1;
static const int16_t PIP_R = 3;
static const uint8_t MAX_POINTS = 8;

struct Outline
{
    uint8_t count;
    int16_t xs[MAX_POINTS];
    int16_t ys[MAX_POINTS];
    int16_t labelY; // Where the number sits: low in a triangle, high in a kite
};

static void regular(Outline &o, uint8_t count, float startDegrees, int16_t cy)
{
    o.count = count;
    for (uint8_t i = 0; i < count; i++)
    {
        float a = (startDegrees + 360.0f * i / count) * (float)M_PI / 180.0f;
        o.xs[i] = (int16_t)lroundf(C + R * cosf(a));
        o.ys[i] = (int16_t)lroundf(cy + R * sinf(a));
    }
    o.labelY = cy;
}

static void points(Outline &o, std::initializer_list<int16_t> coords, int16_t labelY)
{
    o.count = 0;
    const int16_t *p = coords.begin();
    for (size_t i = 0; i + 1 < coords.size() && o.count < MAX_POINTS; i += 2)
    {
        o.xs[o.count] = p[i];
        o.ys[o.count] = p[i + 1];
        o.count++;
    }
    o.labelY = labelY;
}

// The silhouette each die shows lying on the table
static bool outlineOf(uint8_t sides, Outline &o)
{
    const int16_t E = DieFace::SIZE - 1;
    switch (sides)
    {
    case 4:
        points(o, {C, 1, E, E - 2, 0, E - 2}, 23);
        return true;
    case 8:
        points(o, {C, 0, E, C, C, E, 0, C}, C);
        return true;
    case 10:
        points(o, {C, 0, E, 14, C, E, 0, 14}, 15);
        return true;
    case 12:
        regular(o, 5, -90, C + 1);
        return true;
    case 20:
        regular(o, 6, -90, C);
        return true;
    case 100:
        regular(o, 8, -67.5f, C);
        return true;
    default:
        return false; // The coin's circle and the d6's square are drawn as such
    }
}

void DieFace::drawPips(TFT_eSPI &tft, uint16_t color)
{
    // Cells of a 3x3 grid read row by row, bit 0 the top left
    static const uint16_t PIPS[6] = {0x010, 0x044, 0x054, 0x145, 0x155, 0x16D};
    uint16_t cells = PIPS[value - 1];
    for (uint8_t cell = 0; cell < 9; cell++)
    {
        if (cells & (1 << cell))
        {
            int16_t px = x + 9 + (cell % 3) * 9;
            int16_t py = y + 9 + (cell / 3) * 9;
            tft.fillCircle(px, py, PIP_R, color);
        }
    }
}

void DieFace::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);

    bool blank = value == 0;
    Color body, ink;
    switch (style)
    {
    case ASIDE:
        body = Theme::FRAME_DIM;
        ink = Theme::TEXT_DIM;
        break;
    case GOOD:
        body = Theme::GOOD;
        ink = Theme::CARD;
        break;
    case BAD:
        body = Theme::BAD;
        ink = Theme::CARD;
        break;
    case ROLLING:
        body = Theme::TEXT_2;
        ink = Theme::CARD;
        break;
    default:
        body = Theme::TEXT;
        ink = Theme::CARD;
        break;
    }
    if (blank)
    {
        body = Theme::FRAME;
    }

    Outline o;
    int16_t labelY = C;
    if (outlineOf(sides, o))
    {
        for (uint8_t i = 1; i + 1 < o.count; i++)
        {
            tft.fillTriangle(x + o.xs[0], y + o.ys[0], x + o.xs[i], y + o.ys[i], x + o.xs[i + 1], y + o.ys[i + 1], body);
        }
        labelY = o.labelY;
    }
    else if (sides == 6)
    {
        tft.fillRoundRect(x + 1, y + 1, SIZE - 2, SIZE - 2, 6, body);
    }
    else
    {
        tft.fillCircle(x + C, y + C, R, body);
    }

    if (blank)
    {
        return;
    }
    if (sides == 6 && value <= 6)
    {
        drawPips(tft, ink);
        return;
    }

    char label[4];
    snprintf(label, sizeof(label), "%u", (unsigned)value);
    tft.setTextFont(1);
    tft.setTextSize(value >= 100 ? 1 : 2);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(ink);
    tft.drawString(label, x + C + 1, y + labelY + 1);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
}
