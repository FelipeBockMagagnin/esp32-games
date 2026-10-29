#include "WheelView.h"

static const int16_t CX = WheelView::SIZE / 2;
static const int16_t RADIUS = 80;
static const int16_t CY = WheelView::SIZE - RADIUS - 2;
static const int16_t HUB_R = 10;
static const int16_t POINTER_HALF_W = 8;
static const int16_t POINTER_H = CY - RADIUS + 4;
static const float LABEL_RADIUS = RADIUS * 0.68f;
static const float MAX_ARC_STEP = 6.0f;
static const uint8_t DIM_ALPHA = 90;

// Neighbours stay apart in every theme
static constexpr Color SLICE_COLORS[] = {Theme::GOLD, Theme::INFO, Theme::BAD, Theme::GOOD,
                                         Theme::EPIC, Theme::MYTHIC, Theme::DEFENSE};
static const uint8_t SLICE_COLOR_COUNT = sizeof(SLICE_COLORS) / sizeof(SLICE_COLORS[0]);

WheelView::WheelView(TFT_eSPI &tft, const Spinner &spinner, int16_t x, int16_t y)
    : UI(x, y, SIZE, SIZE), canvas(&tft), spinner(spinner)
{
}

Color WheelView::sliceColor(uint8_t slice, uint8_t slices)
{
    uint8_t index = slice % SLICE_COLOR_COUNT;
    // The last slice touches the first
    if (slice == slices - 1 && slice > 0 && index == 0)
    {
        index = 1;
    }
    return SLICE_COLORS[index];
}

void WheelView::release()
{
    if (allocated)
    {
        canvas.deleteSprite();
        allocated = false;
    }
}

static void pointAt(float degrees, float radius, int16_t &px, int16_t &py)
{
    float a = degrees * (float)M_PI / 180.0f;
    px = (int16_t)lroundf(CX + radius * sinf(a));
    py = (int16_t)lroundf(CY - radius * cosf(a));
}

static uint16_t inkOn(uint16_t color)
{
    uint32_t r = (color >> 11) * 255 / 31, g = (color >> 5 & 63) * 255 / 63, b = (color & 31) * 255 / 31;
    return (r * 299 + g * 587 + b * 114) / 1000 > 140 ? TFT_BLACK : TFT_WHITE;
}

void WheelView::paint(TFT_eSPI &g, int16_t ox, int16_t oy)
{
    uint16_t bg = eraseColor;
    g.fillRect(ox, oy, SIZE, SIZE, bg);

    uint8_t slices = spinner.getSlices();
    float width = 360.0f / slices;
    float turn = spinner.getAngle();
    uint8_t steps = (uint8_t)ceilf(width / MAX_ARC_STEP);
    for (uint8_t i = 0; i < slices; i++)
    {
        uint16_t color = sliceColor(i, slices);
        if (winner >= 0 && i != winner)
        {
            color = g.alphaBlend(DIM_ALPHA, color, bg);
        }
        float start = turn + i * width;
        int16_t px, py;
        pointAt(start, RADIUS, px, py);
        for (uint8_t s = 1; s <= steps; s++)
        {
            int16_t nx, ny;
            pointAt(start + width * s / steps, RADIUS, nx, ny);
            g.fillTriangle(ox + CX, oy + CY, ox + px, oy + py, ox + nx, oy + ny, color);
            px = nx;
            py = ny;
        }
    }
    for (uint8_t i = 0; i < slices; i++)
    {
        int16_t px, py;
        pointAt(turn + i * width, RADIUS, px, py);
        g.drawLine(ox + CX, oy + CY, ox + px, oy + py, bg);
    }
    g.drawCircle(ox + CX, oy + CY, RADIUS + 1, Theme::FRAME);

    g.setTextFont(1);
    g.setTextSize(2);
    g.setTextDatum(MC_DATUM);
    for (uint8_t i = 0; i < slices; i++)
    {
        uint16_t color = sliceColor(i, slices);
        bool dim = winner >= 0 && i != winner;
        int16_t lx, ly;
        pointAt(turn + (i + 0.5f) * width, LABEL_RADIUS, lx, ly);
        char label[4];
        snprintf(label, sizeof(label), "%u", (unsigned)i + 1);
        uint16_t ink = inkOn(dim ? g.alphaBlend(DIM_ALPHA, color, bg) : color);
        g.setTextColor(ink);
        g.drawString(label, ox + lx + 1, oy + ly + 1);
    }
    g.setTextSize(1);
    g.setTextDatum(TL_DATUM);

    g.fillCircle(ox + CX, oy + CY, HUB_R, Theme::CARD);
    g.drawCircle(ox + CX, oy + CY, HUB_R, Theme::FRAME);

    g.fillTriangle(ox + CX - POINTER_HALF_W, oy, ox + CX + POINTER_HALF_W, oy, ox + CX, oy + POINTER_H, Theme::TEXT);
    g.drawTriangle(ox + CX - POINTER_HALF_W, oy, ox + CX + POINTER_HALF_W, oy, ox + CX, oy + POINTER_H, Theme::BG);
}

void WheelView::draw(TFT_eSPI &tft)
{
    if (!allocated)
    {
        canvas.setColorDepth(16);
        allocated = canvas.createSprite(SIZE, SIZE) != nullptr;
    }
    if (!allocated)
    {
        paint(tft, x, y);
        return;
    }
    paint(canvas, 0, 0);
    canvas.pushSprite(x, y);
}
