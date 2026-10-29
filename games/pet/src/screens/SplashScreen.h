#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include "Screen.h"
#include "../ui/SplashArt.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// The dotcompany logo and a jingle, opened as a modal at boot and closed after a few
// seconds or at the first press
class SplashScreen : public Screen
{
public:
    SplashScreen(ScreenManager &screenManager, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onConfirmPress() override { close(); }
    void onUpPress() override { close(); }
    void onDownPress() override { close(); }
    bool onBackPress() override
    {
        close();
        return true;
    }

private:
    SplashArt art;
    ScreenManager &screenManager;
    SoundManager &sound;
    unsigned long start;

    void close();
};

#endif // SPLASH_SCREEN_H
