#ifndef UI_ORE_DISPLAY_H
#define UI_ORE_DISPLAY_H

#include "UI.h"

// Ore image that shakes when mined, throwing off chips, with "+N" popups floating up from it.
// Everything is composed in an off-screen sprite and pushed at once, so the
// popups can pass over the ore without flicker or erasing holes in it.
//
// The ore sits on a soft glow with a shadow under it, and can be recolored: pixels
// matching the source palette are swapped for the target one, so one sprite serves
// every ore tier. Black pixels in the image are treated as transparent.
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
    void addPopup(const String &text, uint16_t color);
    // Chips fly off the ore and fall; in the ore's own colours, or gold sparks
    void burst(uint8_t count, bool gold);
    // Coins arc from the ore toward (targetX, targetY), relative to the element and clamped
    // into it, one after another
    void flyCoins(uint8_t count, int16_t targetX, int16_t targetY);
    // How many coins reached their target since the last call
    uint8_t takeArrivals();

    // Both arrays hold `count` RGB565 colors and must outlive the element (point into
    // constexpr tables). Pass a null target to draw the image in its own colors.
    void setPalette(const uint16_t *source, const uint16_t *target, uint8_t count);

    // Tint of the halo behind the ore; blended toward the background so it stays dim
    void setGlowColor(uint16_t color);

    // A gold vein beside the ore: the icon (16x16 RGB565) at 2x with a pulsing glow and the
    // Up button under it, on the left or right of the ore, blinking when `fading`
    void setVein(const uint16_t *icon, bool leftSide, bool fading);
    void clearVein();

    // Call every loop: advances the animations and marks the element dirty while they run
    void update(unsigned long now);

    void draw(TFT_eSPI &tft) override;

private:
    static const uint8_t MAX_POPUPS = 5;
    static const uint8_t MAX_CHIPS = 20;
    static const uint8_t MAX_COINS = 12;

    struct Popup
    {
        bool active;
        unsigned long start;
        int16_t x;
        uint16_t color;
        String text;
    };

    struct Chip
    {
        bool active;
        unsigned long start;
        int16_t x;
        int16_t y;
        int16_t vx; // Pixels per second
        int16_t vy;
        uint16_t color;
    };

    const uint16_t *orePixels;
    int16_t oreX;
    int16_t oreY;
    int16_t oreW;
    int16_t oreH;
    uint16_t popupColor;

    const uint16_t *paletteSource;
    const uint16_t *paletteTarget;
    uint8_t paletteCount;
    uint16_t glowColor;

    struct Coin
    {
        bool active;
        unsigned long start;
        int16_t x0;
        int16_t y0;
        int16_t cx; // Control point of the arc
        int16_t cy;
        int16_t x1;
        int16_t y1;
    };

    Popup popups[MAX_POPUPS];
    uint8_t nextPopup;
    Chip chips[MAX_CHIPS];
    uint8_t nextChip;
    Coin coins[MAX_COINS];
    uint8_t nextCoin;
    uint8_t arrivals;

    bool shaking;
    unsigned long shakeStart;

    const uint16_t *veinIcon; // nullptr while no vein is shown
    bool veinLeft;
    bool veinFading;
    unsigned long lastVeinFrame;

    unsigned long now;
    unsigned long lastFrame;
    bool animating;

    TFT_eSprite *sprite;

    int16_t shakeOffset() const;
    void drawBackdrop(TFT_eSPI &tft);
    void drawOre(int16_t left);
    void drawVein(TFT_eSPI &tft);
    void drawChips();
    void drawCoins();
};

#endif // UI_ORE_DISPLAY_H
