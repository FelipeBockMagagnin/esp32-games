#ifndef GAME_OVER_SCREEN_H
#define GAME_OVER_SCREEN_H

#include "Screen.h"
#include "Text.h"
#include "../core/ScreenManager.h"
#include "../core/SoundManager.h"
#include "../ui/ArtImage.h"

// A modal after a death: how deep the run went, what it killed, and whether that was a record.
// Confirm or Left goes back to the title.
class GameOverScreen : public Screen
{
public:
    GameOverScreen(ScreenManager &screenManager, SoundManager &sound);

    void setResult(uint8_t floor, uint16_t kills, bool record);

    void onConfirmPress() override;
    bool onBackPress() override;
    bool onRightPress() override { return true; }

private:
    ScreenManager &screenManager;
    SoundManager &sound;

    Text titleText;
    Text floorText;
    Text killsText;
    Text recordText;
    Text hintText;
    ArtImage skull;
};

#endif // GAME_OVER_SCREEN_H
