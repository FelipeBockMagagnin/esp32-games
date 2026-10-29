#include "FieldRow.h"
#include "ButtonBadge.h"

static const int16_t PAD = 8;
static const int16_t TEXT_DY = 3;
static const int16_t ARROW_HALF_W = 4;
static const int16_t ARROW_H = 5;
static const int16_t CAP_R = 6;
static const int16_t CAP_GAP = 6;

void FieldRow::draw(TFT_eSPI &tft)
{
    bool lit = state != NORMAL;
    tft.fillRect(x, y, w, h, lit ? Theme::ROW_SELECTED : Theme::CARD);
    tft.drawRect(x, y, w, h, lit ? Theme::SELECT : Theme::FRAME_DIM);

    tft.setTextFont(1);
    tft.setTextSize(2);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(lit ? Theme::TEXT : Theme::TEXT_2);
    tft.drawString(label, x + PAD, y + TEXT_DY);

    int16_t right = x + w - PAD;
    if (state == EDITING)
    {
        int16_t cx = right - ARROW_HALF_W;
        int16_t mid = y + h / 2;
        tft.fillTriangle(cx - ARROW_HALF_W, mid - 2, cx + ARROW_HALF_W, mid - 2, cx, mid - 2 - ARROW_H, Theme::SELECT);
        tft.fillTriangle(cx - ARROW_HALF_W, mid + 2, cx + ARROW_HALF_W, mid + 2, cx, mid + 2 + ARROW_H, Theme::SELECT);
        right -= 2 * ARROW_HALF_W + 6;
    }
    tft.setTextDatum(TR_DATUM);
    Color valueColor = state == EDITING ? Theme::SELECT : (muted ? Theme::TEXT_DIM : Theme::TEXT);
    tft.setTextColor(valueColor);
    tft.drawString(value, right, y + TEXT_DY);
    tft.setTextSize(1);
    tft.setTextDatum(TL_DATUM);
}

void ActionRow::draw(TFT_eSPI &tft)
{
    tft.fillRect(x, y, w, h, selected ? Theme::ROW_SELECTED : Theme::CARD);
    tft.drawRect(x, y, w, h, selected ? Theme::SELECT : Theme::FRAME);

    tft.setTextFont(1);
    tft.setTextSize(2);
    int16_t labelW = tft.textWidth(label);
    int16_t left = x + (w - labelW - 2 * CAP_R - CAP_GAP) / 2;
    ButtonBadge::drawCap(tft, left + CAP_R, y + h / 2, CAP_R, ButtonBadge::CONFIRM, !selected);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(selected ? Theme::TEXT : Theme::TEXT_2);
    tft.drawString(label, left + 2 * CAP_R + CAP_GAP, y + TEXT_DY);
    tft.setTextSize(1);
}
