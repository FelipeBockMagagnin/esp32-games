#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../stack/Game.h"
#include "../stack/Records.h"
#include "../ui/WellView.h"

// The game, opened as a modal from the title. It draws the well straight to the panel from
// update(); only the side panel's text is made of elements. Left and Right repeat, Down is read
// as a level through `keyDown`, Up slams the piece down, a press of Confirm turns it, and
// holding Confirm pauses, a bar filling in the side panel meanwhile.
class PlayScreen : public Screen
{
public:
    PlayScreen(TFT_eSPI &tft, Game &game, Records &records, RandomSource random, void (*save)(),
               bool (*keyDown)(Key), ScreenManager &screenManager, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

private:
    enum class State : uint8_t
    {
        PLAYING,
        PAUSED,
        ENDING, // The stack turns over, a row at a time
        OVER
    };

    TFT_eSPI &tft;
    Game &game;
    Records &records;
    RandomSource random;
    void (*save)();
    bool (*keyDown)(Key);
    ScreenManager &screenManager;
    SoundManager &sound;
    WellView view;

    Text nextLabel;
    Text statLabels[3];
    Text statValues[3];
    Text bestLabel;
    Text bestValue;
    Text chainText;
    Text hintTop;
    Text hintBottom;

    State state = State::PLAYING;
    Mode mode = Mode::ENDLESS;
    unsigned long lastTick = 0;
    unsigned long lastFrame = 0;
    uint8_t wipeStep = 0;
    uint8_t chainShown = 0;
    bool softBlocked = false;

    bool pauseHeld = false;
    unsigned long pauseHeldAt = 0;
    int16_t barShown = 0;

    int8_t endRow = 0;
    unsigned long boxAt = 0;
    uint8_t choice = 0;
    bool newBest = false;
    bool recorded = false;

    void newGame();
    void paint();
    void repaintAll();
    void drawNext();
    void drawBar(int16_t width);
    void refreshPanel();
    void refreshBest();

    void shift(int8_t dx);
    void handleEvents(unsigned long now);
    void stepAnimations(unsigned long now);
    void checkPauseHold(unsigned long now);
    void beginEnding(unsigned long now);
    void recordRun();
    void pause();
    void pick();
    void drawBox();
    uint8_t optionCount() const { return state == State::PAUSED ? 3 : 2; }
};

#endif // PLAY_SCREEN_H
