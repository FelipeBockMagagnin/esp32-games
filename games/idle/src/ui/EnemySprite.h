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
//
// The enemy bobs while idle, flashes and shakes when hit, fades out when killed, and
// damage numbers float up over it. All of it is painted into the same line buffer, in
// frame pixels, so the animations cost one blit per frame and never flicker either.
class EnemySprite : public UI
{
public:
    static const uint8_t SCALE = 2;
    static const int16_t SIZE = SPRITE_FRAME * SCALE;

    EnemySprite(int16_t x = 0, int16_t y = 0, const SpriteDef *sprite = nullptr);

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

    // Animations start at the time of the last update() and only mark the element dirty
    // from update(), so a caller can hold them back while something covers the sprite
    void hit(bool heavy);
    void lunge(); // Jumps at the player as its own hit lands
    void addDamage(uint32_t damage, bool heavy);
    void kill(); // Fades out the enemy shown now, even once setSprite() has moved on
    void stopAnimations();

    void update(unsigned long now);

    void draw(TFT_eSPI &tft) override;

private:
    static const uint8_t MAX_NUMBERS = 4;

    struct DamageNumber
    {
        bool active;
        bool heavy;
        unsigned long start;
        int16_t x; // Frame pixels, left edge
        uint32_t value;
    };

    void drawCountdown(TFT_eSPI &tft);
    void paintNumbers(uint8_t row, uint16_t *line) const;

    const SpriteDef *sprite;
    const SpriteDef *background;
    uint8_t countdown;

    const SpriteDef *dyingSprite; // nullptr unless the death fade is running
    unsigned long deathStart;
    bool hitting;
    bool hitHeavy;
    unsigned long hitStart;
    bool lunging;
    unsigned long lungeStart;
    DamageNumber numbers[MAX_NUMBERS];
    uint8_t nextNumber;
    uint8_t bob;

    unsigned long now;
    unsigned long lastFrame;
    bool animating;
};

#endif // UI_ENEMY_SPRITE_H
