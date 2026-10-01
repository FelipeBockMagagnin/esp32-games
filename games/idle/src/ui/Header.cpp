#include "Header.h"
#include "../assets/Assets.h"
#include "../game/GameConfig.h" // GOLD_COLOR

static const uint16_t COLOR = 0xFFFF;
static const int16_t NAV_Y = 5;
static const int16_t ARROW_Y = 7;
static const int16_t TITLE_Y = 11;

// Notice layout: a small caption above the name, both centered in the same 28px bar
static const int16_t NOTICE_CAPTION_Y = 2;
static const int16_t NOTICE_NAME_Y = 11;

void Header::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, eraseColor);

    if (hasNotice)
    {
        tft.setTextColor(GOLD_COLOR);
        tft.setTextDatum(TC_DATUM);
        tft.setTextSize(1);
        tft.drawString("ACHIEVEMENT", x + w / 2, y + NOTICE_CAPTION_Y);
        tft.setTextSize(2);
        tft.drawString(notice, x + w / 2, y + NOTICE_NAME_Y);
        tft.setTextDatum(TL_DATUM);
        return;
    }

    tft.setTextColor(COLOR);

    if (backLabel.length() > 0)
    {
        tft.drawBitmap(x + 4, y + ARROW_Y, image_ButtonLeftSmall_bits, 3, 5, COLOR);
        tft.setTextSize(1);
        tft.setTextDatum(TL_DATUM);
        tft.drawString(backLabel, x + 10, y + NAV_Y);
    }

    if (nextLabel.length() > 0)
    {
        tft.drawBitmap(x + w - 5, y + ARROW_Y, image_ButtonRightSmall_bits, 3, 5, COLOR);
        tft.setTextSize(1);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(nextLabel, x + w - 7, y + NAV_Y);
    }

    tft.setTextSize(2);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(title, x + w / 2, y + TITLE_Y);

    // Other elements draw with top-left datum and don't reset it
    tft.setTextDatum(TL_DATUM);
}
