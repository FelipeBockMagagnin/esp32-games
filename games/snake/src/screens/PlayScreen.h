#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../snake/Records.h"
#include "../snake/Snake.h"

// The game, opened as a modal from the title. It paints the field cell by cell as the snake
// moves, so it draws straight to the panel from update() rather than through elements; only
// the score line above the field is made of them. Pausing and the end of a game put a box
// over the field, and closing it repaints the field whole.
class PlayScreen : public Screen
{
public:
    PlayScreen(TFT_eSPI &tft, Snake &snake, Records &records, RandomSource random, void (*save)(),
               ScreenManager &screenManager, SoundManager &sound);

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    void onConfirmPress() override;

    bool isPlaying() const { return state == State::PLAYING; }

private:
    enum class State : uint8_t
    {
        READY, // Drawn and waiting for the first press
        PLAYING,
        PAUSED,
        DYING, // The snake blinks before the box comes up
        OVER
    };

    TFT_eSPI &tft;
    Snake &snake;
    Records &records;
    RandomSource random;
    void (*save)();
    ScreenManager &screenManager;
    SoundManager &sound;

    Text scoreText;
    Text levelText;
    Text bestText;

    State state = State::READY;
    uint8_t choice = 0; // The box's highlighted option
    bool won = false;
    bool newBest = false;
    unsigned long lastStep = 0;
    unsigned long nextBlink = 0;
    bool eyesShut = false;
    unsigned long lastPulse = 0;
    bool foodBig = false;
    unsigned long lastFlash = 0;
    uint8_t flashes = 0;
    bool snakeShown = true;

    void newGame();
    void press(Dir d);
    void advance(unsigned long now);
    void die();
    void finish();
    void refreshScore();
    void showLevel();

    void cellOrigin(Cell c, int16_t &px, int16_t &py) const;
    void drawField();
    void drawBorder();
    void drawSegment(uint16_t i);
    void drawEmpty(Cell c);
    void drawFood();
    void drawSnake(bool shown);
    void drawBox();
};

#endif // PLAY_SCREEN_H
