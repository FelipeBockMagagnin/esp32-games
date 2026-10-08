#ifndef UI_ENEMY_SPRITE_H
#define UI_ENEMY_SPRITE_H

#include "UI.h"
#include "../assets/Sprites.h"

// Draws a palette + run-length SpriteDef at an integer scale over an optional background
// in the same format, decoding one row of each at a time into a small line buffer instead
// of keeping a full RGB565 image in flash. Without a background, transparent pixels take
// the erase colour. Either way the whole frame goes out in a single window, so changing
// the enemy never flickers. A countdown is painted into the same draw, over the frame,
// since a separate Text could only erase it with a hole in the art.
class EnemySprite : public UI
{
public:
    static const uint8_t SCALE = 2;
    static const int16_t SIZE = SPRITE_FRAME * SCALE;

    EnemySprite(int16_t x = 0, int16_t y = 0, const SpriteDef *sprite = nullptr)
        : UI(x, y, SIZE, SIZE), sprite(sprite), background(nullptr), countdown(0)
    {
    }

    void setSprite(const SpriteDef *newSprite)
    {
        if (sprite != newSprite)
        {
            sprite = newSprite;
            markDirty();
        }
    }

    void setBackground(const SpriteDef *newBackground)
    {
        if (background != newBackground)
        {
            background = newBackground;
            markDirty();
        }
    }

    // Seconds until the next enemy, shown on a card in the middle; 0 for none
    void setCountdown(uint8_t seconds)
    {
        if (countdown != seconds)
        {
            countdown = seconds;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    void drawCountdown(TFT_eSPI &tft);

    const SpriteDef *sprite;
    const SpriteDef *background;
    uint8_t countdown;
};

#endif // UI_ENEMY_SPRITE_H
