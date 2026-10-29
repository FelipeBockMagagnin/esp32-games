#ifndef UI_SPLASH_ART_H
#define UI_SPLASH_ART_H

#include "UI.h"
#include "../assets/Sprites.h"

// The dotcompany logo: the hooded character, bobbing between its two frames, with the
// wordmark across its chest. Both are composited into one line buffer, like EnemySprite,
// so a frame change is a single blit and the wordmark never flickers over the character.
class SplashArt : public UI
{
public:
    static const uint8_t FRAME_COUNT = 2;

    SplashArt(int16_t x, int16_t y);

    void setFrame(uint8_t index)
    {
        if (frame != index)
        {
            frame = index;
            markDirty();
        }
    }

    void draw(TFT_eSPI &tft) override;

private:
    uint8_t frame;
    int16_t characterLeft;
    int16_t wordmarkLeft;
};

#endif // UI_SPLASH_ART_H
