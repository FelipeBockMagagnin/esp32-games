#ifndef DRAW_SCREEN_H
#define DRAW_SCREEN_H

#include "ToolScreen.h"
#include "Box.h"
#include "Text.h"
#include "../dice/History.h"
#include "../dice/Prefs.h"
#include "../dice/Raffle.h"
#include "../ui/Chip.h"

// A number between two bounds, with or without repeats, over the last numbers out of the hat
class DrawScreen : public ToolScreen
{
public:
    DrawScreen(Prefs &prefs, Raffle &raffle, History &history, RandomSource source, SoundManager &sound,
               void (*saved)());

    void update(unsigned long now) override;
    void onEnter(TFT_eSPI &tft) override;
    void onExit() override;

protected:
    void refreshFields() override;
    bool changeField(uint8_t field, int16_t delta) override;
    int16_t bigStep(uint8_t field, int8_t direction) const override;
    void act() override;
    bool isBusy() const override { return shuffling; }

private:
    enum Field : uint8_t
    {
        FIELD_FROM,
        FIELD_TO,
        FIELD_REPEATS,
        FIELD_TOTAL
    };
    static const uint8_t RECENT = 15;

    Box card;
    Text numberText;
    Text captionText;
    Chip chips[RECENT];

    Prefs &prefs;
    Raffle &raffle;
    History &history;
    RandomSource source;
    int16_t recent[RECENT]; // Newest first
    uint8_t recentCount = 0;
    bool shuffling = false;
    unsigned long shuffleStart = 0;
    unsigned long lastShuffleFrame = 0;

    void applyRange();
    void clearRecent();
    void settle();
    void show();
};

#endif // DRAW_SCREEN_H
