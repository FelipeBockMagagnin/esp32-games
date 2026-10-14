#include "Header.h"
#include "../assets/Assets.h"
#include "Theme.h"

static const uint16_t COLOR = Theme::TEXT;
static const uint16_t NAV_COLOR = Theme::TEXT_2; // The neighbours are secondary to the title
static const uint16_t BAND_COLOR = Theme::HEADER; // A step below the screen background
static const uint16_t RULE_COLOR = Theme::FRAME;  // Same frame as the cards

static const int16_t NAV_Y = 5;
static const int16_t ARROW_Y = 7;
static const int16_t TITLE_Y = 10; // Size 2 glyphs end above the rule on the bottom row

// Notice layout: a small caption above the name, both centered in the same 28px bar
static const int16_t NOTICE_CAPTION_Y = 2;
static const int16_t NOTICE_NAME_Y = 11;

static const unsigned long FLASH_STEP_MS = 70;
static const uint8_t FLASH_ALPHA_PER_STEP = 30;

// A slanted band: each row down shifts it right by one pixel every SHINE_SLANT rows
static const unsigned long SHINE_MS = 650;
static const unsigned long SHINE_STEP_MS = 30;
static const int16_t SHINE_W = 14;
static const int16_t SHINE_SLANT = 2;
static const uint8_t SHINE_ALPHA = 90;

void Header::update(unsigned long now)
{
    if (flashLevel > 0 && now - lastFlashStep >= FLASH_STEP_MS)
    {
        lastFlashStep = now;
        flashLevel--;
        invalidate();
        if (flashLevel == 0 && hasNotice)
        {
            shining = true;
            shineStart = now;
            shineX = -SHINE_W - h / SHINE_SLANT;
        }
        return;
    }

    if (shining && now - lastFlashStep >= SHINE_STEP_MS)
    {
        lastFlashStep = now;
        unsigned long age = now - shineStart;
        int16_t start = -SHINE_W - h / SHINE_SLANT;
        shineX = start + (int32_t)(w - start) * (int32_t)(age < SHINE_MS ? age : SHINE_MS) / SHINE_MS;
        if (age >= SHINE_MS)
        {
            shining = false;
        }
        if (!dirty)
        {
            shineOnly = true;
        }
        markDirty();
    }
}

void Header::paintShine(TFT_eSPI &tft, uint16_t band)
{
    if (!shining)
    {
        return;
    }
    uint16_t glow = tft.alphaBlend(SHINE_ALPHA, noticeAccent, band);
    for (int16_t row = 0; row < h - 1; row++)
    {
        tft.drawFastHLine(x + shineX + row / SHINE_SLANT, y + row, SHINE_W, glow);
    }
}

// The shine only moved: repaint the columns it left and entered, with the text clipped to them
void Header::draw(TFT_eSPI &tft)
{
    uint16_t band = flashLevel > 0 ? tft.alphaBlend(flashLevel * FLASH_ALPHA_PER_STEP, noticeAccent, BAND_COLOR)
                                   : BAND_COLOR;
    bool partial = shineOnly && drawn && hasNotice;
    shineOnly = false;
    if (partial)
    {
        int16_t left = (drawnShineX < shineX ? drawnShineX : shineX);
        int16_t right = (drawnShineX > shineX ? drawnShineX : shineX) + SHINE_W + h / SHINE_SLANT;
        left = left < 0 ? 0 : left;
        right = right > w ? w : right;
        drawnShineX = shineX;
        if (right <= left)
        {
            return;
        }
        tft.setViewport(x + left, y, right - left, h - 1, false);
        paintBar(tft, band);
        tft.resetViewport();
        return;
    }
    drawnShineX = shineX;
    paintBar(tft, band);
    tft.drawFastHLine(x, y + h - 1, w, hasNotice ? noticeAccent : RULE_COLOR);
}

void Header::paintBar(TFT_eSPI &tft, uint16_t band)
{
    tft.fillRect(x, y, w, h - 1, band);

    if (hasNotice)
    {
        paintShine(tft, band);
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(1);
        if (noticeCaption != nullptr)
        {
            tft.setTextColor(noticeAccent);
            tft.drawString(noticeCaption, x + w / 2, y + NOTICE_CAPTION_Y);
        }
        tft.setTextColor(COLOR);
        tft.setTextSize(2);
        tft.drawString(notice, x + w / 2, y + NOTICE_NAME_Y);
        tft.setTextDatum(TL_DATUM);
        return;
    }

    tft.setTextSize(1);
    tft.setTextColor(NAV_COLOR);

    if (backLabel.length() > 0)
    {
        tft.drawBitmap(x + 4, y + ARROW_Y, image_ButtonLeftSmall_bits, 3, 5, NAV_COLOR);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(backLabel, x + 10, y + NAV_Y);
    }

    if (nextLabel.length() > 0)
    {
        tft.drawBitmap(x + w - 7, y + ARROW_Y, image_ButtonRightSmall_bits, 3, 5, NAV_COLOR);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(nextLabel, x + w - 10, y + NAV_Y);
    }

    tft.setTextColor(COLOR);
    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(title, x + w / 2, y + TITLE_Y);

    // Other elements draw with top-left datum and don't reset it
    tft.setTextDatum(TL_DATUM);
}
