#ifndef GAME_OVER_SCREEN_H
#define GAME_OVER_SCREEN_H

#include "Screen.h"
#include "PlayScreen.h"
#include "Box.h"
#include "../ui/Header.h"
#include "ListRow.h"
#include "../ui/Text.h"

// The Unity end panel, as a modal over the frozen field: the round's points and coins, then
// play again or back to the title (Left too)
class GameOverScreen : public Screen
{
public:
    GameOverScreen(const Round &round, const Profile &profile, PlayScreen &play, ScreenManager &screenManager,
                   SoundManager &sound);

    // Before opening it: whether the round just banked set a new best
    void setNewBest(bool best) { newBest = best; }

    void onEnter(TFT_eSPI &tft) override;
    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
    bool onBackPress() override;

private:
    enum Row : uint8_t
    {
        ROW_AGAIN,
        ROW_TITLE,
        ROW_COUNT
    };

    Header header;
    Box card;
    Text pointsText;
    Text bestText;
    Text earnedText;
    ListRow rows[ROW_COUNT];

    const Round &round;
    const Profile &profile;
    PlayScreen &play;
    ScreenManager &screenManager;
    SoundManager &sound;
    bool newBest = false;
    uint8_t selected = 0;

    void refreshRows();
};

#endif // GAME_OVER_SCREEN_H
