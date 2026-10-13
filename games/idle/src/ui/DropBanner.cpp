#include "DropBanner.h"
#include "../game/ItemConfig.h"

static const int16_t RADIUS = 6;
static const int16_t CAPTION_DY = 8;
static const int16_t ICON_DY = 30;
static const int16_t NAME_DY = 98;
static const int16_t SUB_DY = 117;
static const int16_t OTHERS_GAP = 6;

static uint16_t swapBytes(uint16_t color)
{
    return (uint16_t)((color >> 8) | (color << 8));
}

void DropBanner::draw(TFT_eSPI &tft)
{
    const ItemDef &def = ITEMS[itemId];
    uint16_t rarity = ITEM_TIER_COLORS[def.tier];
    int16_t cx = x + w / 2;

    tft.fillRoundRect(x, y, w, h, RADIUS, Theme::CARD);
    tft.drawRoundRect(x, y, w, h, RADIUS, rarity);
    tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, RADIUS - 1, rarity);

    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(isNew ? Theme::GOOD : Theme::GOLD);
    tft.drawString(isNew ? "NEW ITEM!" : "LEVEL UP!", cx, y + CAPTION_DY);

    // The 16x16 icon blown up 4x, one source row at a time; its transparent black takes
    // the card's color
    if (def.iconPixels != nullptr && def.iconW <= 16)
    {
        int16_t size = def.iconW * ICON_SCALE;
        uint16_t line[16 * ICON_SCALE];
        uint16_t card = swapBytes(Theme::CARD);
        tft.startWrite();
        tft.setAddrWindow(cx - size / 2, y + ICON_DY, size, def.iconH * ICON_SCALE);
        for (int16_t row = 0; row < def.iconH; row++)
        {
            for (int16_t col = 0; col < def.iconW; col++)
            {
                uint16_t c = pgm_read_word(def.iconPixels + row * def.iconW + col);
                for (uint8_t k = 0; k < ICON_SCALE; k++)
                {
                    line[col * ICON_SCALE + k] = c == 0 ? card : c;
                }
            }
            for (uint8_t k = 0; k < ICON_SCALE; k++)
            {
                tft.pushPixels(line, size);
            }
        }
        tft.endWrite();
    }

    char buf[32];
    if (others > 0)
    {
        snprintf(buf, sizeof(buf), "+%u", (unsigned)others);
        tft.setTextDatum(ML_DATUM);
        tft.setTextColor(Theme::GOLD);
        tft.drawString(buf, cx + 16 * ICON_SCALE / 2 + OTHERS_GAP, y + ICON_DY + 16 * ICON_SCALE / 2);
        tft.setTextDatum(TC_DATUM);
    }

    tft.setTextColor(rarity);
    tft.drawString(def.name, cx, y + NAME_DY);

    if (isNew)
    {
        snprintf(buf, sizeof(buf), "%s %s", ITEM_TIER_NAMES[def.tier], EQUIP_SLOT_NAMES[(uint8_t)def.slot]);
    }
    else
    {
        snprintf(buf, sizeof(buf), "now level %u", (unsigned)level);
    }
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_2);
    tft.drawString(buf, cx, y + SUB_DY);
    tft.setTextDatum(TL_DATUM);
}

// Clears only what the sprite will not cover when it redraws
void DropBanner::erase(TFT_eSPI &tft)
{
    int16_t right = drawnX + drawnW;
    int16_t bottom = drawnY + drawnH;
    int16_t holeRight = holeX + holeW;
    int16_t holeBottom = holeY + holeH;

    if (holeY > drawnY)
    {
        tft.fillRect(drawnX, drawnY, drawnW, holeY - drawnY, eraseColor);
    }
    if (bottom > holeBottom)
    {
        tft.fillRect(drawnX, holeBottom, drawnW, bottom - holeBottom, eraseColor);
    }
    if (holeX > drawnX)
    {
        tft.fillRect(drawnX, holeY, holeX - drawnX, holeH, eraseColor);
    }
    if (right > holeRight)
    {
        tft.fillRect(holeRight, holeY, right - holeRight, holeH, eraseColor);
    }
}
