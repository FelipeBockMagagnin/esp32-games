#ifndef UI_DROP_BANNER_H
#define UI_DROP_BANNER_H

#include "UI.h"

// A big card announcing an item drop: a caption, the item's icon at 4x, its name in its
// rarity color, and what it is. A kill that dropped several shows its rarest, with a
// "+N" beside the icon for the rest. It sits over the enemy sprite while it is up.
//
// Two elements cannot erase each other cleanly (see UI.h), so the card is built around
// the sprite instead: hiding it only clears its area *outside* the sprite's rectangle
// (the "hole"), and the screen redraws the sprite, which covers the rest.
class DropBanner : public UI
{
public:
    static const uint8_t ICON_SCALE = 4;

    DropBanner(int16_t x, int16_t y, int16_t w, int16_t h, int16_t holeX, int16_t holeY, int16_t holeW, int16_t holeH)
        : UI(x, y, w, h), holeX(holeX), holeY(holeY), holeW(holeW), holeH(holeH),
          itemId(0), isNew(false), level(0), others(0)
    {
        visible = false;
    }

    void show(uint8_t newItemId, bool newIsNew, uint8_t newLevel, uint8_t newOthers)
    {
        itemId = newItemId;
        isNew = newIsNew;
        level = newLevel;
        others = newOthers;
        visible = true;
        markDirty();
    }

    void draw(TFT_eSPI &tft) override;

protected:
    void erase(TFT_eSPI &tft) override;

private:
    int16_t holeX;
    int16_t holeY;
    int16_t holeW;
    int16_t holeH;
    uint8_t itemId;
    bool isNew;
    uint8_t level;
    uint8_t others;
};

#endif // UI_DROP_BANNER_H
