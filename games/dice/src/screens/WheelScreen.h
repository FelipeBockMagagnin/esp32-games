#ifndef WHEEL_SCREEN_H
#define WHEEL_SCREEN_H

#include "ToolScreen.h"
#include "Text.h"
#include "../dice/History.h"
#include "../dice/Prefs.h"
#include "../dice/Spinner.h"
#include "../ui/WheelView.h"

// A wheel of numbered slices that spins and slows under a pointer, ticking past each peg
class WheelScreen : public ToolScreen
{
public:
    WheelScreen(TFT_eSPI &tft, Prefs &prefs, Spinner &spinner, History &history, RandomSource source,
                SoundManager &sound, void (*saved)());

    void update(unsigned long now) override;
    void onEnter(TFT_eSPI &tft) override;
    void onExit() override;

protected:
    void refreshFields() override;
    bool changeField(uint8_t field, int16_t delta) override;
    int16_t bigStep(uint8_t field, int8_t direction) const override { return 3; }
    void act() override;
    bool isBusy() const override { return spinner.isSpinning(); }

private:
    WheelView wheel;
    Text resultText;
    Text captionText;

    Prefs &prefs;
    Spinner &spinner;
    History &history;
    RandomSource source;
    bool hasResult = false;
    uint16_t heardCrossings = 0;
    unsigned long lastFrame = 0;
    unsigned long lastClick = 0;

    void finish();
    void show();
};

#endif // WHEEL_SCREEN_H
