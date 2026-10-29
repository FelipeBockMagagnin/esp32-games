#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../mines/Field.h"
#include "../mines/Records.h"
#include "../ui/FieldView.h"

// The game, opened as a modal from the title. It paints the field tile by tile, so it draws
// straight to the panel from update() rather than through elements; only the counters in the
// HUD are made of them. The cursor walks the field and, past its top row, the pause button
// in the HUD. A tap of Confirm opens the tile (or, on a number, its neighbours); holding it
// plants or lifts a flag, a ring filling round the cursor meanwhile, so the screen reads the
// button's level through `confirmDown`.
class PlayScreen : public Screen
{
public:
    PlayScreen(TFT_eSPI &tft, Field &field, Records &records, RandomSource random, void (*save)(),
               bool (*confirmDown)(), ScreenManager &screenManager, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    void onConfirmPress() override;
    bool onRepeat(Key key) override;

    bool isPlaying() const { return state == State::PLAYING; }
    Cell getCursor() const { return cursor; }
    void moveTo(Cell c);

private:
    enum class State : uint8_t
    {
        PLAYING,
        PAUSED,
        BLAST,   // The mine that went off flashes
        CHAIN,   // Then the others turn up, nearest first
        WINNING, // Flags rise over the mines and the field glints
        OVER
    };

    TFT_eSPI &tft;
    Field &field;
    Records &records;
    RandomSource random;
    void (*save)();
    bool (*confirmDown)();
    ScreenManager &screenManager;
    SoundManager &sound;
    FieldView view;

    Text minesText;
    Text timeText;

    State state = State::PLAYING;
    uint8_t level = 0;
    Cell cursor = {0, 0};
    bool onButton = false;
    bool tight = false;
    unsigned long lastPulse = 0;

    bool holding = false;
    unsigned long holdStart = 0;
    uint8_t ringDots = 0;

    unsigned long lastReveal = 0;
    bool flagAnimating = false;
    Cell flagCell = {0, 0};
    uint8_t flagFrame = 0;
    unsigned long lastFlagFrame = 0;

    unsigned long startedAt = 0;
    unsigned long pausedAt = 0;
    uint16_t finalTenths = 0;
    bool timing = false;

    unsigned long lastFrame = 0;
    uint8_t frames = 0;
    Cell chain[Field::MAX_CELLS];
    uint16_t chainLength = 0;
    uint16_t chainShown = 0;
    unsigned long chainMs = 0;
    uint8_t sweepAt = 0;

    uint8_t choice = 0;
    bool newBest = false;

    void newGame();
    uint16_t elapsedTenths(unsigned long now) const;
    void refreshMines();
    void refreshTime(unsigned long now);

    void move(int8_t dx, int8_t dy);
    void tap(unsigned long now);
    void hold();
    void settle(Field::Result result, unsigned long now);
    void stopClock(unsigned long now);
    void blast(unsigned long now);
    void win(unsigned long now);
    void finish();
    void pause();

    void drawHud();
    void drawButton();
    void drawCursorCell();
    void drawBox();
    uint8_t optionCount() const { return state == State::PAUSED ? 3 : 2; }
    void pick();
};

#endif // PLAY_SCREEN_H
