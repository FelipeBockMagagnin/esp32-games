#ifndef FOCUS_SCREEN_H
#define FOCUS_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/TimerRing.h"
#include "../ui/CycleDots.h"
#include "../app/FocusTimer.h"
#include "../managers/SoundManager.h"

// The pomodoro itself: Confirm starts or pauses, Up restarts the phase, Down skips it
class FocusScreen : public Screen
{
public:
    FocusScreen(FocusTimer &timer, SoundManager &sound);

    void update(unsigned long now) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;

private:
    Header header;
    TimerRing ring;
    Text phaseText;
    Text timeText;
    Text stateText;
    CycleDots dots;
    Text hintText;
    Text hintText2;

    FocusTimer &timer;
    SoundManager &sound;
};

#endif // FOCUS_SCREEN_H
