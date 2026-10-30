#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "Screen.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// The boot screen: the flag, the name, and three blocks running along a bar, until a press or
// the timeout closes it
class SplashScreen : public Screen
{
public:
    SplashScreen(ScreenManager &screenManager, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onConfirmPress() override { finish(); }
    bool onBackPress() override { return true; }
    bool onRightPress() override { return true; }

private:
    class Logo : public UI
    {
    public:
        Logo() : UI(0, 0, DISPLAY_W, DISPLAY_H) {}
        void draw(TFT_eSPI &tft) override;
    };

    class Runner : public UI
    {
    public:
        Runner();
        void setStep(uint8_t value)
        {
            if (step != value)
            {
                step = value;
                markDirty();
            }
        }
        void draw(TFT_eSPI &tft) override;

    private:
        uint8_t step = 0;
    };

    ScreenManager &screenManager;
    SoundManager &sound;
    Logo logo;
    Runner runner;
    unsigned long start = 0;
    bool started = false;

    void finish();
};

#endif // SPLASH_SCREEN_H
