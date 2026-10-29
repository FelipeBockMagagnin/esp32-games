#ifndef LEVEL_SCREEN_H
#define LEVEL_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "Box.h"
#include "../game/Stage.h"
#include "../game/SaveGame.h"
#include "../render/Renderer.h"
#include "../managers/ScreenManager.h"
#include "../managers/SoundManager.h"

// A floor being played: the stage stepped at 60 Hz whatever the frame rate, the play area pushed
// in DMA bands under a one-line HUD, then the coworker's dialogue and the floor's results.
// Up pauses. Every overlay (cards, pause, dialogue) is drawn by UI elements over a frozen frame.
class LevelScreen : public Screen
{
public:
    LevelScreen(ScreenManager &screenManager, SoundManager &sound, SaveBlob &records, Screen &settingsScreen,
                void (*quit)());

    // Starts a floor from its intro card; call before switching to this screen
    void begin(uint8_t level);
    // The buttons held this loop; presses arrive through the handlers
    void setHeld(bool left, bool right, bool jump);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void render(TFT_eSPI &tft) override;

    void onConfirmPress() override;
    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override;
    bool onNextPress() override { return true; }
    // Keeps the hold-Left Settings gesture off while Left runs
    bool repeatsArrows() const override { return true; }
    bool onArrowRepeat(Arrow arrow) override { return false; }

private:
    enum class Mode : uint8_t
    {
        INTRO,
        PLAY,
        PAUSE,
        TALK,
        OUTRO
    };

    static const uint8_t PAUSE_ROWS = 4;
    static const uint8_t CARD_LINES = 5;

    ScreenManager &screenManager;
    SoundManager &sound;
    SaveBlob &records;
    Screen &settingsScreen;
    void (*quit)();

    Stage stage;
    Renderer renderer;
    Mode mode;
    unsigned long modeSince;
    unsigned long lastStepAt;
    uint16_t stepCredit; // Thirds of a millisecond owed to the simulation
    bool held[3];
    bool jumpLatch;
    bool dashLatch;
    bool frameDue;
    bool clearPlayArea;
    uint8_t pauseRow;
    uint8_t page;
    bool newBest;

    Box hudBar;
    Text floorText;
    Text timeText;
    Text bossText;
    Text deathText;

    Text cardLines[CARD_LINES];

    Box pausePanel;
    Text pauseRows[PAUSE_ROWS];

    Box talkBox;
    Text speakerText;
    Text talkLines[2];
    Text talkHint;

    void setMode(Mode m, unsigned long now);
    void showCard(bool intro);
    void showPause(bool show);
    void showTalk(bool show);
    void refreshPause();
    void showPage();
    void refreshHud();
    void playEvents(uint16_t events);
    void finishFloor(unsigned long now);
    void pushFrame(TFT_eSPI &tft);
    // Takes the overlay off without erasing it, since the next frame paints over it anyway
    static void lift(UI &element);
};

#endif // LEVEL_SCREEN_H
