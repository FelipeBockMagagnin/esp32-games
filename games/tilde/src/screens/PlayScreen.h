#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include "Screen.h"
#include "Game.h"
#include "../art/Spring.h"

// The minigames. Hum: the pet sings a tune on the buzzer, one note per arrow, and the player
// sings it back, a note longer each round. Catch: treats fall and the pet walks under them.
// While one runs it takes all four arrows.
class PlayScreen : public Screen
{
public:
    PlayScreen(Display &display, Game &game) : Screen(display), game(game) {}

    void onEnter(TFT_eSPI &tft) override;
    void update(unsigned long now) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override;
    bool onRepeat(Key key) override;
    bool allowsGestures() const override { return state == State::MENU; }

protected:
    const char *title() const override { return "Play"; }
    void drawScene(Canvas &c) const override;

private:
    enum class State : uint8_t
    {
        MENU,
        HUM_LISTEN, // the pet sings
        HUM_SING,   // the player sings back
        HUM_MISS,
        CATCH,
        RESULT
    };
    enum MiniGame : uint8_t
    {
        HUM,
        CATCH_GAME,
        GAME_COUNT
    };

    static const uint8_t HUM_MAX = 32;
    static const uint8_t TREAT_COUNT = 5;

    struct Treat
    {
        float x, y, vy;
        uint8_t kind;
        bool falling;
    };

    Game &game;
    State state = State::MENU;
    uint8_t selected = 0;
    uint8_t score = 0;
    bool newBest = false;
    unsigned long stateAt = 0;
    unsigned long nextAt = 0;
    unsigned long lastUpdate = 0;

    uint8_t tune[HUM_MAX];
    uint8_t tuneLength = 0;
    uint8_t position = 0;
    int8_t lit = -1;
    unsigned long litAt = 0;
    unsigned long litUntil = 0;

    Treat treats[TREAT_COUNT];
    float catcherX = 120;
    float catcherTo = 120;
    Spring squash = Spring(220, 12);
    float fallSpeed = 0;
    unsigned long nextSpawn = 0;
    unsigned long caughtAt = 0;
    uint8_t rewardJoy = 0;
    uint8_t rewardFood = 0;

    void start(unsigned long now);
    void startRound(unsigned long now);
    void sing(uint8_t arrow, unsigned long now);
    void finish(unsigned long now);
    void moveCatcher(int8_t dir);
    void updateCatch(unsigned long now, float dt);
    void drawMenu(Canvas &c) const;
    void drawHum(Canvas &c) const;
    void drawCatch(Canvas &c) const;
    void drawResult(Canvas &c) const;
};

#endif // PLAY_SCREEN_H
