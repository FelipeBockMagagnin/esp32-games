#ifndef UI_STAR_RISE_H
#define UI_STAR_RISE_H

#include "UI.h"

// Sparks rising through the element's area, each a 2x2 dot that erases where it was. Nothing
// else may share the area.
class StarRise : public UI
{
public:
    StarRise(int16_t x, int16_t y, int16_t w, int16_t h);

    void start(unsigned long now);
    void update(unsigned long now);
    void draw(TFT_eSPI &tft) override;

private:
    static const uint8_t COUNT = 18;
    static const int16_t DOT = 2;

    struct Star
    {
        int16_t x;
        int16_t y;      // Pixels from the top of the area; drawn where it was last frame
        int16_t drawnX;
        int16_t drawnY; // -1 when not on the panel
        uint8_t speed;  // Pixels per frame
        uint8_t color;  // Index into the palette
    };

    void respawn(Star &star);

    Star stars[COUNT];
    unsigned long lastFrame;
    bool running;
};

#endif // UI_STAR_RISE_H
