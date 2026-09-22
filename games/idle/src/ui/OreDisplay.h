#ifndef UI_ORE_DISPLAY_H
#define UI_ORE_DISPLAY_H

#include "UI.h"

// Ore image that shakes when mined, with "+N" popups floating up from it.
// Everything is composed in an off-screen sprite and pushed at once, so the
// popups can pass over the ore without flicker or erasing holes in it.
class OreDisplay : public UI
{
public:
    // (x, y, w, h) is the animated area; the ore is drawn at (oreX, oreY) inside it
    OreDisplay(int16_t x, int16_t y, int16_t w, int16_t h,
               const uint16_t *orePixels, int16_t oreX, int16_t oreY, int16_t oreW, int16_t oreH,
               uint16_t popupColor);
    ~OreDisplay();

    // Animations start at the time of the last update(), so their age can never be negative
    void shake();
    void addPopup(const String &text);

    // Call every loop: advances the animations and marks the element dirty while they run
    void update(unsigned long now);

    void draw(TFT_eSPI &tft) override;

private:
    static const uint8_t MAX_POPUPS = 5;

    struct Popup
    {
        bool active;
        unsigned long start;
        int16_t x;
        String text;
    };

    const uint16_t *orePixels;
    int16_t oreX;
    int16_t oreY;
    int16_t oreW;
    int16_t oreH;
    uint16_t popupColor;

    Popup popups[MAX_POPUPS];
    uint8_t nextPopup;

    bool shaking;
    unsigned long shakeStart;

    unsigned long now;
    unsigned long lastFrame;
    bool animating;

    TFT_eSprite *sprite;

    int16_t shakeOffset() const;
};

#endif // UI_ORE_DISPLAY_H
