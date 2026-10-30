#ifndef CLOCK_SCREEN_H
#define CLOCK_SCREEN_H

#include "Screen.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../pet/Pet.h"
#include "../ui/Panel.h"

// The clock's hour and minute, asked at every boot: the board forgets the time when powered off,
// so the last saved one is offered and Confirm twice keeps it. Time stands still here.
class ClockScreen : public Screen
{
public:
    ClockScreen(Pet &pet, LcdMatrix &lcd, Panel &panel, void (*changed)(), ScreenManager &screenManager,
                SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
    bool onRepeat(Key key) override;
    bool onBackPress() override { return true; }
    bool onRightPress() override { return true; }

private:
    Pet &pet;
    LcdMatrix &lcd;
    Panel &panel;
    void (*changed)();
    ScreenManager &screenManager;
    SoundManager &sound;
    int8_t hour = 12;
    int8_t minute = 0;
    bool onMinutes = false;
    bool blink = false;
    unsigned long nextBlinkAt = 0;

    void step(int8_t delta);
    void drawMatrix();
};

#endif // CLOCK_SCREEN_H
