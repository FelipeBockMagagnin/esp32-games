#include "Panel.h"
#include "Theme.h"

void Panel::drawIcon(TFT_eSPI &tft, uint8_t icon, bool on)
{
    const Sprite &art = ICON_ART[icon];
    int16_t x = ICON_X + (icon % 4) * ICON_PITCH;
    int16_t y = icon < 4 ? ICON_TOP_Y : ICON_BOTTOM_Y;
    uint16_t color = on ? Theme::INK : Theme::GHOST;
    tft.fillRect(x, y, art.w * ICON_DOT, art.h * ICON_DOT, Theme::FIELD);
    for (int16_t j = 0; j < art.h; j++)
    {
        for (int16_t i = 0; i < art.w; i++)
        {
            if (art.art[j * art.w + i] == '#')
            {
                tft.fillRect(x + i * ICON_DOT, y + j * ICON_DOT, ICON_DOT, ICON_DOT, color);
            }
        }
    }
}

void Panel::paint(TFT_eSPI &tft)
{
    tft.fillScreen(Theme::SHELL);
    tft.fillRoundRect(5, 20, tft.width() - 10, tft.height() - 40, 14, Theme::FIELD);
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(Theme::FIELD, Theme::SHELL);
    tft.setTextFont(1);
    tft.drawString("BOBA", tft.width() / 2, 7);
    shownSelected = NONE;
    shownCalling = false;
    for (uint8_t i = 0; i < ICON_COUNT; i++)
    {
        drawIcon(tft, i, false);
    }
    lcd.invalidate();
}

void Panel::setIcons(TFT_eSPI &tft, int8_t selected, bool calling)
{
    if (selected != shownSelected)
    {
        if (shownSelected != NONE)
        {
            drawIcon(tft, (uint8_t)shownSelected, false);
        }
        if (selected != NONE)
        {
            drawIcon(tft, (uint8_t)selected, true);
        }
        shownSelected = selected;
    }
    if (calling != shownCalling)
    {
        drawIcon(tft, ICON_ATTENTION, calling);
        shownCalling = calling;
    }
}
