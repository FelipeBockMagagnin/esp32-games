#include "ListRow.h"
#include "IconUtil.h"
#include "Theme.h"

static const uint16_t SELECTED_COLOR = Theme::SELECT;
static const uint16_t OWNED_COLOR = Theme::GOOD;
static const uint16_t DIMMED_COLOR = Theme::TEXT_DIM;
static const uint16_t SUBTITLE_COLOR = Theme::TEXT_2; // Secondary to the title

// Icon cell on the left, then the text column; values are right-aligned
static const int16_t ICON_CELL_W = 26;
static const int16_t TEXT_DX = 31;
static const int16_t VALUE_MARGIN = 5;
static const int16_t COIN_RADIUS = 3;
static const uint8_t DIM_ICON_PERCENT = 55; // Brightness a locked row's gray icon keeps
static const uint16_t FLASH_COLOR = Theme::GOOD;
static const uint8_t FLASH_FILL_ALPHA = 90;
static const int16_t TREND_HALF_W = 3;

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

// Triangles rather than the GLCD font's arrow glyphs, which sit below the printable range
static void drawTriangle(TFT_eSPI &tft, int16_t cx, int16_t top, int16_t height, bool up, uint16_t color)
{
    int16_t bottom = top + height - 1;
    if (up)
    {
        tft.fillTriangle(cx - TREND_HALF_W, bottom, cx + TREND_HALF_W, bottom, cx, top, color);
    }
    else
    {
        tft.fillTriangle(cx - TREND_HALF_W, top, cx + TREND_HALF_W, top, cx, bottom, color);
    }
}

void ListRow::draw(TFT_eSPI &tft)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }

    bool highlighted = state == SELECTED && hasSelectedFill;
    uint16_t fill = highlighted ? selectedFill : fillColor;
    if (flashOn)
    {
        fill = tft.alphaBlend(FLASH_FILL_ALPHA, FLASH_COLOR, fill);
    }
    tft.fillRect(x, y, w, h, fill);

    uint16_t frame = frameColor;
    uint16_t titleColor = Theme::TEXT;
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

    tft.drawRect(x, y, w, h, flashOn ? FLASH_COLOR : frame);

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
            tft.drawEllipse(coinX, y + 6, COIN_RADIUS, COIN_RADIUS, Theme::GOLD);
        }

        int16_t trendX = valueRight - tft.textWidth(value) - TREND_HALF_W - 4;
        if (state != DIMMED && (trend == TREND_UP || trend == TREND_DOWN))
        {
            drawTriangle(tft, trendX, y + 3, 7, trend == TREND_UP, trend == TREND_UP ? Theme::GOOD : Theme::BAD);
        }
        else if (state != DIMMED && trend == TREND_MIXED)
        {
            drawTriangle(tft, trendX, y + 2, 4, true, Theme::GOOD);
            drawTriangle(tft, trendX, y + 7, 4, false, Theme::BAD);
        }
    }

    if (valueSub.length() > 0)
    {
        tft.setTextColor(state == DIMMED ? DIMMED_COLOR : valueSubColor);
        tft.drawString(valueSub, valueRight, y + h - 9);
    }

    tft.setTextDatum(TL_DATUM);
}
