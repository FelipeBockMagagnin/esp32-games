#include "DropBanner.h"
#include "../game/ItemStats.h"

static const int16_t RADIUS = 6;
static const int16_t CAPTION_DY = 8;
static const int16_t ICON_DY = 30;
static const int16_t NAME_DY = 98;
static const int16_t SUB_DY = 117;
static const int16_t OTHERS_GAP = 6;

// Level-up layout: a small icon beside the name, then what the level changed and a bar
// toward the next stat that will rise
static const uint8_t SMALL_ICON_SCALE = 2;
static const int16_t PAD_X = 12;
static const int16_t SMALL_ICON_DY = 30;
static const int16_t LEVEL_NAME_DY = 32;
static const int16_t LEVEL_LINE_DY = 52;
static const int16_t STAT_LINES_DY = 68;
static const int16_t STAT_LINE_H = 10;
static const uint8_t MAX_STAT_LINES = 4;
static const int16_t GAIN_BAR_DY = 110;
static const int16_t GAIN_BAR_H = 12;

static uint16_t swapBytes(uint16_t color)
{
    return (uint16_t)((color >> 8) | (color << 8));
}

// The 16x16 icon blown up, one source row at a time; its transparent black takes the
// card's color
static void drawIcon(TFT_eSPI &tft, const ItemDef &def, int16_t left, int16_t top, uint8_t scale)
{
    if (def.iconPixels == nullptr || def.iconW > 16)
    {
        return;
    }
    int16_t size = def.iconW * scale;
    uint16_t line[16 * DropBanner::ICON_SCALE];
    uint16_t card = swapBytes(Theme::CARD);
    tft.startWrite();
    tft.setAddrWindow(left, top, size, def.iconH * scale);
    for (int16_t row = 0; row < def.iconH; row++)
    {
        for (int16_t col = 0; col < def.iconW; col++)
        {
            uint16_t c = pgm_read_word(def.iconPixels + row * def.iconW + col);
            for (uint8_t k = 0; k < scale; k++)
            {
                line[col * scale + k] = c == 0 ? card : c;
            }
        }
        for (uint8_t k = 0; k < scale; k++)
        {
            tft.pushPixels(line, size);
        }
    }
    tft.endWrite();
}

void DropBanner::draw(TFT_eSPI &tft)
{
    const ItemDef &def = ITEMS[itemId];
    uint16_t rarity = Theme::tierColor(def.tier);
    int16_t cx = x + w / 2;

    tft.fillRoundRect(x, y, w, h, RADIUS, Theme::CARD);
    tft.drawRoundRect(x, y, w, h, RADIUS, rarity);
    tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, RADIUS - 1, rarity);

    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(isNew ? Theme::GOOD : Theme::GOLD);
    tft.drawString(isNew ? "NEW ITEM!" : "LEVEL UP!", cx, y + CAPTION_DY);
    if (!isNew)
    {
        drawLevelUp(tft, def, rarity);
        tft.setTextDatum(TL_DATUM);
        return;
    }

    drawIcon(tft, def, cx - def.iconW * ICON_SCALE / 2, y + ICON_DY, ICON_SCALE);

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

    snprintf(buf, sizeof(buf), "%s %s", ITEM_TIER_NAMES[def.tier], EQUIP_SLOT_NAMES[(uint8_t)def.slot]);
    tft.setTextSize(1);
    tft.setTextColor(Theme::TEXT_2);
    tft.drawString(buf, cx, y + SUB_DY);
    tft.setTextDatum(TL_DATUM);
}

// Every levelled value the item has, the ones this level raised with their change; then a
// bar from the level the soonest-rising value last rose at to the level it rises next
void DropBanner::drawLevelUp(TFT_eSPI &tft, const ItemDef &def, uint16_t rarity)
{
    int16_t left = x + PAD_X;
    int16_t right = x + w - PAD_X;
    int16_t textX = left + def.iconW * SMALL_ICON_SCALE + 8;
    uint8_t before = level > 1 ? level - 1 : level;
    char buf[40];
    char change[16];

    drawIcon(tft, def, left, y + SMALL_ICON_DY, SMALL_ICON_SCALE);

    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(2);
    tft.setTextColor(rarity);
    tft.drawString(def.name, textX, y + LEVEL_NAME_DY);

    tft.setTextSize(1);
    snprintf(buf, sizeof(buf), "Lv %u > %u", (unsigned)before, (unsigned)level);
    tft.setTextColor(Theme::GOLD);
    tft.drawString(buf, textX, y + LEVEL_LINE_DY);
    if (others > 0)
    {
        snprintf(buf, sizeof(buf), "+%u more", (unsigned)others);
        tft.setTextDatum(TR_DATUM);
        tft.drawString(buf, right, y + LEVEL_LINE_DY);
    }

    uint8_t lines = 0;
    auto drawLine = [&](const char *text, bool rose, const char *delta) {
        int16_t lineY = y + STAT_LINES_DY + lines * STAT_LINE_H;
        tft.setTextDatum(TL_DATUM);
        tft.setTextColor(rose ? Theme::TEXT : Theme::TEXT_DIM);
        tft.drawString(text, left, lineY);
        if (rose)
        {
            tft.setTextDatum(TR_DATUM);
            tft.setTextColor(Theme::GOOD);
            tft.drawString(delta, right, lineY);
        }
        lines++;
    };

    for (uint8_t k = 0; k < STAT_KIND_COUNT && lines < MAX_STAT_LINES; k++)
    {
        StatKind kind = (StatKind)k;
        if (!hasStat(def, kind))
        {
            continue;
        }
        int32_t now = statAtLevel(def, kind, level);
        int32_t was = statAtLevel(def, kind, before);
        formatStat(buf, sizeof(buf), STAT_LABELS[kind], kind, now, isPercentStat(kind) || kind == STAT_REGEN);
        formatStat(change, sizeof(change), "", kind, now - was, true);
        drawLine(buf, now != was, change);
    }

    int16_t barY = y + GAIN_BAR_DY;
    int16_t innerW = right - left - 2;
    NextGain gain = findNextGain(def, level);
    int16_t fillW = innerW;
    if (gain.found)
    {
        uint8_t span = level + gain.levels - gain.sinceLevel;
        fillW = (int16_t)((int32_t)innerW * (level - gain.sinceLevel) / span);
        formatNextGain(buf, sizeof(buf), gain, STAT_LABELS);
    }
    else
    {
        snprintf(buf, sizeof(buf), level >= ITEM_MAX_LEVEL ? "MAX LEVEL" : "No stat rises further");
    }
    tft.fillRect(left + 1, barY + 1, fillW, GAIN_BAR_H - 2, Theme::GOLD_BAR);
    tft.fillRect(left + 1 + fillW, barY + 1, innerW - fillW, GAIN_BAR_H - 2, Theme::GOLD_TRACK);
    tft.drawRect(left, barY, right - left, GAIN_BAR_H, Theme::GOLD_BORDER);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::TEXT);
    tft.drawString(buf, (left + right) / 2, barY + GAIN_BAR_H / 2);
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
