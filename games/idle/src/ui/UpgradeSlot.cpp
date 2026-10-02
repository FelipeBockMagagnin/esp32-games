#include "UpgradeSlot.h"

static const int16_t RADIUS = 4;
static const uint16_t SELECTED_COLOR = 0xFFE0; // Yellow, the same selection color as list rows
static const uint16_t DEAR_FRAME = 0x4208;     // Dark gray
static const uint16_t DEAR_FILL = 0x10A2;      // Near-black gray
static const uint8_t TINT_ALPHA = 48;          // Accent blended over the background for the fill
static const uint8_t SELECTED_TINT_ALPHA = 80;
static const uint8_t DEAR_ICON_ALPHA = 90;     // How much of the icon survives the fade

static uint16_t swapBytes(uint16_t color)
{
    return (uint16_t)((color >> 8) | (color << 8));
}

void UpgradeSlot::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);

    uint16_t fill;
    uint16_t frame;
    switch (state)
    {
    case SELECTED:
        fill = tft.alphaBlend(SELECTED_TINT_ALPHA, accent, eraseColor);
        frame = SELECTED_COLOR;
        break;
    case TOO_DEAR:
        fill = DEAR_FILL;
        frame = DEAR_FRAME;
        break;
    default:
        fill = tft.alphaBlend(TINT_ALPHA, accent, eraseColor);
        frame = accent;
        break;
    }

    tft.fillRoundRect(x, y, w, h, RADIUS, fill);
    tft.drawRoundRect(x, y, w, h, RADIUS, frame);
    if (state == SELECTED)
    {
        tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, RADIUS - 1, frame);
    }

    if (icon == nullptr)
    {
        return;
    }

    // Composed in a small buffer: transparent pixels take the tile's fill, and a too-dear
    // icon is faded toward it. Asset words are byte-swapped, as pushImage sends them raw.
    uint16_t buffer[ICON_SIZE * ICON_SIZE];
    for (int16_t i = 0; i < ICON_SIZE * ICON_SIZE; i++)
    {
        uint16_t raw = pgm_read_word(icon + i);
        uint16_t color = raw == 0 ? fill : swapBytes(raw);
        if (raw != 0 && state == TOO_DEAR)
        {
            color = tft.alphaBlend(DEAR_ICON_ALPHA, color, fill);
        }
        buffer[i] = swapBytes(color);
    }
    tft.pushImage(x + (w - ICON_SIZE) / 2, y + (h - ICON_SIZE) / 2, ICON_SIZE, ICON_SIZE, buffer);
}
