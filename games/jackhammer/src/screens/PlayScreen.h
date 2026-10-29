#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../jack/Round.h"
#include "../jack/Profile.h"
#include "../render/ArenaRenderer.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"

// The game: a HUD over the square arena, which is pushed in DMA bands. On the title the hammer
// stands in its style's start particles, Confirm starts a round and Left/Right go round the
// screens; in a round Left and Right turn it (Confirm turns clockwise, like the Unity game's
// Space). A death freezes the field a moment, then `died` runs.
class PlayScreen : public Screen
{
public:
    static const int16_t ARENA_TOP = 80;

    PlayScreen(Round &round, const Profile &profile, SoundManager &sound, void (*died)());

    void toTitle();
    void newRound();
    // After the style changed in the shop
    void styleChanged();

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override;

private:
    enum class Mode : uint8_t
    {
        TITLE,
        PLAY,
        DYING
    };

    Text titleText;
    Text titleBest;
    Text titleCoins;
    Text musicHint;
    Text styleHint;
    Text multiplierText;
    Text scoreText;
    Text coinText;
    Text bestText;

    Round &round;
    const Profile &profile;
    SoundManager &sound;
    void (*died)();

    ArenaRenderer renderer;
    Effects effects;
    Rng rng{0xC0FFEE};
    Mode mode = Mode::TITLE;
    uint8_t backgroundIndex = 0;
    uint8_t tick = 0;
    unsigned long lastStepAt = 0;
    uint32_t stepCredit = 0;
    unsigned long dyingSince = 0;
    bool deathReported = false;
    bool frameDue = true;

    const StyleArt &style() const { return STYLE_ART[profile.style]; }
    void startTitleParticles();
    void turn(int8_t direction);
    void playEvents();
    void nextBackground();
    void refreshText();
    void pushFrame(TFT_eSPI &tft);
};

#endif // PLAY_SCREEN_H
