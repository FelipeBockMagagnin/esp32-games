#include "GameOverScreen.h"

static const int16_t CENTER = DISPLAY_W / 2;

GameOverScreen::GameOverScreen(ScreenManager &screenManager, SoundManager &sound)
    : screenManager(screenManager),
      sound(sound),
      titleText(CENTER, 24, "YOU DIED", Theme::BAD, 4, TC_DATUM),
      floorText(CENTER, 142, "", Theme::TEXT, 2, TC_DATUM),
      killsText(CENTER, 166, "", Theme::TEXT_2, 1, TC_DATUM),
      recordText(CENTER, 182, "", Theme::GOLD, 1, TC_DATUM),
      hintText(CENTER, 222, "Confirm to rise again", Theme::TEXT_DIM, 1, TC_DATUM),
      skull(CENTER - 32, 66, &ENEMY_ART[ART_ENEMY_SKELETON], 1)
{
    addElement(&skull);
    addElement(&titleText);
    addElement(&floorText);
    addElement(&killsText);
    addElement(&recordText);
    addElement(&hintText);
}

void GameOverScreen::setResult(uint8_t floor, uint16_t kills, bool record)
{
    char text[32];
    snprintf(text, sizeof(text), "Fell on floor %u", floor);
    floorText.setText(text);
    snprintf(text, sizeof(text), "%u slain", kills);
    killsText.setText(text);
    recordText.setText(record ? "Deeper than ever before" : "");
}

void GameOverScreen::onConfirmPress()
{
    sound.playMenu();
    screenManager.closeModal();
}

bool GameOverScreen::onBackPress()
{
    onConfirmPress();
    return true;
}
