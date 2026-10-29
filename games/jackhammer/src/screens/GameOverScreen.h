#ifndef GAME_OVER_SCREEN_H
#define GAME_OVER_SCREEN_H

#include "Screen.h"
#include "PlayScreen.h"
#include "../ui/Box.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/Text.h"

// The Unity end panel, as a modal over the frozen field: the round's points and coins, then
// continue (once a round, for coins instead of an ad), play again or back to the title
class GameOverScreen : public Screen
{
public:
    // `pay` takes the continue's price from the coins and saves; false when they fall short
    GameOverScreen(const Round &round, const Profile &profile, PlayScreen &play, ScreenManager &screenManager,
                   SoundManager &sound, bool (*pay)(uint32_t price));

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
        ROW_CONTINUE,
        ROW_AGAIN,
        ROW_TITLE,
        ROW_COUNT
    };

    Header header;
    Box card;
    Text pointsText;
    Text bestText;
    Text earnedText;
    Text totalText;
    ListRow rows[ROW_COUNT];
    Text hintText;

    const Round &round;
    const Profile &profile;
    PlayScreen &play;
    ScreenManager &screenManager;
    SoundManager &sound;
    bool (*pay)(uint32_t price);
    bool newBest = false;
    uint8_t selected = 0;

    bool canContinue() const;
    void refreshRows();
};

#endif // GAME_OVER_SCREEN_H
