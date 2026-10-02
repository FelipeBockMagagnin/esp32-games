#include "ListRow.h"
#include "../game/GameConfig.h"
#include "IconUtil.h"

static const uint16_t SELECTED_COLOR = 0xFFE0; // Yellow
static const uint16_t OWNED_COLOR = 0x07E0;    // Green
static const uint16_t DIMMED_COLOR = 0x7BEF;   // Gray
static const uint16_t SUBTITLE_COLOR = 0xAD55; // Light gray, secondary to the title

// Icon cell on the left, then the text column; values are right-aligned
static const int16_t ICON_CELL_W = 26;
static const int16_t TEXT_DX = 31;
static const int16_t VALUE_MARGIN = 5;
static const int16_t COIN_RADIUS = 3;
static const uint8_t DIM_ICON_PERCENT = 55; // Brightness a locked row's gray icon keeps

void ListRow::setIconBitmap(const unsigned char *bitmap, int16_t bw, int16_t bh, uint16_t color)
{
    if (iconBitmap == bitmap && iconPixels == nullptr && iconW == bw && iconH == bh && iconColor == color)
    {
        return;
    }
    iconBitmap = bitmap;
    iconPixels = nullptr;
    iconW = bw;
    iconH = bh;
    iconColor = color;
    markDirty();
}

void ListRow::setIconPixels(const uint16_t *pixels, int16_t pw, int16_t ph)
{
    if (iconPixels == pixels && iconBitmap == nullptr && iconW == pw && iconH == ph)
    {
        return;
    }
    iconPixels = pixels;
    iconBitmap = nullptr;
    iconW = pw;
    iconH = ph;
    markDirty();
}

void ListRow::clearIcon()
{
    if (iconBitmap == nullptr && iconPixels == nullptr)
    {
        return;
    }
    iconBitmap = nullptr;
    iconPixels = nullptr;
    iconW = 0;
    iconH = 0;
    markDirty();
}

void ListRow::draw(TFT_eSPI &tft)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }

    bool highlighted = state == SELECTED && hasSelectedFill;
    tft.fillRect(x, y, w, h, highlighted ? selectedFill : eraseColor);

    uint16_t frame = frameColor;
    uint16_t titleColor = 0xFFFF;
    uint16_t subColor = SUBTITLE_COLOR;

    switch (state)
    {
    case SELECTED:
        frame = SELECTED_COLOR;
        break;
    case OWNED:
        frame = OWNED_COLOR;
        titleColor = OWNED_COLOR;
        break;
    case DIMMED:
        frame = DIMMED_COLOR;
        titleColor = DIMMED_COLOR;
        subColor = DIMMED_COLOR;
        break;
    default:
        break;
    }

    if (hasTitleColor && state != DIMMED)
    {
        titleColor = titleColorOverride;
    }

    tft.drawRect(x, y, w, h, frame);

    if (iconW > 0 && iconH > 0)
    {
        int16_t iconX = x + (ICON_CELL_W - iconW) / 2;
        int16_t iconY = y + (h - iconH) / 2;
        if (iconPixels != nullptr && state == DIMMED)
        {
            // A locked row's color icon goes gray, like its 1-bit icons and text do
            pushIconGray(tft, iconX, iconY, iconW, iconH, iconPixels, DIM_ICON_PERCENT);
        }
        else if (iconPixels != nullptr)
        {
            // Black is transparent, so an icon on a highlighted row keeps the highlight around it
            pushIcon(tft, iconX, iconY, iconW, iconH, iconPixels);
        }
        else if (iconBitmap != nullptr)
        {
            tft.drawBitmap(iconX, iconY, iconBitmap, iconW, iconH, state == DIMMED ? DIMMED_COLOR : iconColor);
        }
    }

    tft.setTextDatum(TL_DATUM);

    if (title.length() > 0)
    {
        tft.setTextColor(titleColor);
        tft.setTextSize(2);
        tft.drawString(title, x + TEXT_DX, y + 2);
    }

    tft.setTextSize(1);

    if (subtitle.length() > 0)
    {
        tft.setTextColor(subColor);
        tft.drawString(subtitle, x + TEXT_DX, y + h - 9);
    }

    int16_t valueRight = x + w - VALUE_MARGIN;
    tft.setTextDatum(TR_DATUM);

    if (value.length() > 0)
    {
        tft.setTextColor(state == DIMMED ? DIMMED_COLOR : valueColor);
        tft.drawString(value, valueRight, y + 3);

        if (showCoin)
        {
            int16_t coinX = valueRight - tft.textWidth(value) - COIN_RADIUS - 3;
            tft.drawEllipse(coinX, y + 6, COIN_RADIUS, COIN_RADIUS, GOLD_COLOR);
        }
    }

    if (valueSub.length() > 0)
    {
        tft.setTextColor(state == DIMMED ? DIMMED_COLOR : SUBTITLE_COLOR);
        tft.drawString(valueSub, valueRight, y + h - 9);
    }

    tft.setTextDatum(TL_DATUM);
}
