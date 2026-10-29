#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include "Screen.h"
#include "Box.h"
#include "Image.h"
#include "ProgressBar.h"
#include "Text.h"
#include "../core/SoundManager.h"
#include "../crawl/Run.h"
#include "../crawl/Tutorial.h"
#include "../render/Frame.h"
#include "../render/View.h"

// The run itself, a modal: the view on top, sent in bands straight to the panel from update(), and
// the HUD's elements under it. It takes every arrow, so it reads the buttons held (setHeld) rather
// than their presses: Up and Down walk, Left and Right turn, Confirm strikes or charges, and Up with
// Down together pauses.
class PlayScreen : public Screen
{
public:
    struct Hooks
    {
        void (*descended)(); // A new floor began: time to save
        void (*died)();      // A moment after the killing blow
        void (*paused)();
        void (*taught)(); // The tutorial ended, done or walked away from
    };

    PlayScreen(Run &run, Frame &frame, SoundManager &sound, const Hooks &hooks);

    // Before opening it for a run just started or restored; `teach` runs the tutorial when the
    // run stands at the start of the first floor
    void begin(bool teach);
    void setHeld(bool up, bool down, bool left, bool right, bool confirm);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    bool onBackPress() override { return true; }
    bool onRightPress() override { return true; }

    const View &getView() const { return view; }

private:
    static const unsigned long NOTICE_MS = 2600;
    static const unsigned long DEATH_PAUSE_MS = 1400;
    static const uint16_t MAX_STEP_MS = 50;

    Run &run;
    Frame &frame;
    SoundManager &sound;
    Hooks hooks;
    View view;
    Tutorial tutorial;

    Box rule;
    ProgressBar hpBar;
    ProgressBar manaBar;
    Image hpPotionIcon;
    Image mpPotionIcon;
    Text hpPotionText;
    Text mpPotionText;
    Text floorText;
    Text killsText;
    Image weaponIcon;
    Image focusIcon;
    Text weaponText;
    Text focusText;
    Text noticeText;

    Input held = {};
    bool armed = false; // Off until the buttons that opened the screen are let go
    unsigned long lastUpdate = 0;
    unsigned long noticeUntil = 0;
    unsigned long diedAt = 0;
    bool noticePending = false;

    void handleEvents(uint32_t events, unsigned long now);
    void notice(const char *text, Color color, unsigned long now);
    void noticeItem(const char *verb, const Item &item, unsigned long now);
    void refreshHud(unsigned long now);
};

#endif // PLAY_SCREEN_H
