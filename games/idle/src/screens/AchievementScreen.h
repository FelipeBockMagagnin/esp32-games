#ifndef ACHIEVEMENT_SCREEN_H
#define ACHIEVEMENT_SCREEN_H

#include "Screen.h"
#include "../ui/Text.h"
#include "../ui/Header.h"
#include "../ui/ListRow.h"
#include "../ui/ListView.h"
#include "../assets/Assets.h"
#include "../game/Achievements.h"
#include "../managers/SoundManager.h"

// Lists every achievement, unlocked or not, with the running total of how much gold
// production they have earned. Select cycles the list, Confirm jumps to the next one
// still locked, which beats pressing Select through 35 rows.
class AchievementScreen : public Screen
{
private:
    Header header;

    ListRow rows[ListView::VISIBLE_ROWS];
    ListView list;

    Text countText;
    Text bonusText;

    Achievements &achievements;
    SoundManager &sound;

    unsigned long lastRefresh;
    uint8_t lastUnlockedCount;

public:
    AchievementScreen(Achievements &achievements, SoundManager &sound);

    void update(unsigned long now) override;

    void onUpPress() override;
    void onDownPress() override;
    void onConfirmPress() override;
};

#endif // ACHIEVEMENT_SCREEN_H
