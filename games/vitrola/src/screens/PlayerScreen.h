#ifndef PLAYER_SCREEN_H
#define PLAYER_SCREEN_H

#include "Screen.h"
#include "../app/Transport.h"
#include "../core/ScreenManager.h"
#include "../ui/LcdPanel.h"
#include "../ui/Turntable.h"
#include "../ui/VuMeter.h"

// The record player itself. OK plays and pauses, Left and Right change track, Up the play mode,
// Down opens the record shelf.
class PlayerScreen : public Screen
{
public:
    PlayerScreen(Transport &transport, Turntable &turntable, ScreenManager &screenManager, Screen &library);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override;
    bool onRightPress() override;

private:
    static const unsigned long FRAME_MS = 33;

    Transport &transport;
    Turntable &turntable;
    ScreenManager &screenManager;
    Screen &library;
    LcdPanel lcd;
    VuMeter vu;
    unsigned long now = 0;
    unsigned long lastFrame = 0;
    bool fresh = true;

    void drawHints(TFT_eSPI &tft);
};

#endif // PLAYER_SCREEN_H
