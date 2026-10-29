#ifndef ROLL_SCREEN_H
#define ROLL_SCREEN_H

#include "ToolScreen.h"
#include "Box.h"
#include "Text.h"
#include "../dice/History.h"
#include "../dice/Prefs.h"
#include "../dice/Roll.h"
#include "../ui/DieFace.h"

// The dice: how many, which, a bonus and a keep, above the total and every die drawn. A roll
// tumbles for a moment before it shows, though it was decided, and logged, on the press.
class RollScreen : public ToolScreen
{
public:
    RollScreen(Prefs &prefs, History &history, RandomSource source, SoundManager &sound, void (*saved)());

    void update(unsigned long now) override;
    void onEnter(TFT_eSPI &tft) override;
    void onExit() override;

protected:
    void refreshFields() override;
    bool changeField(uint8_t field, int16_t delta) override;
    int16_t bigStep(uint8_t field, int8_t direction) const override;
    void act() override;
    bool isBusy() const override { return tumbling; }

private:
    enum Field : uint8_t
    {
        FIELD_COUNT,
        FIELD_DIE,
        FIELD_BONUS,
        FIELD_KEEP,
        FIELD_TOTAL
    };

    Box card;
    Box table; // Cleared whole when the dice move, since a moved die would erase its neighbour
    Text totalText;
    Text captionText;
    DieFace dice[MAX_DICE];

    Prefs &prefs;
    History &history;
    RandomSource source;
    RollResult result;
    bool hasResult = false;
    bool tumbling = false;
    unsigned long tumbleStart = 0;
    unsigned long lastTumbleFrame = 0;

    void layoutDice();
    void showTumble();
    void showResult();
    void settle();
};

#endif // ROLL_SCREEN_H
