#include "ResetScreen.h"
#include "../game/SaveGame.h"

static const uint16_t WARNING_COLOR = Theme::BAD;
static const uint16_t HINT_COLOR = Theme::TEXT_DIM;

static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 44;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 64;
static const int16_t CARD_CENTER_X = CARD_X + CARD_W / 2;

static const int16_t ROW_X = 5;
static const int16_t NO_ROW_Y = 128;
static const int16_t YES_ROW_Y = NO_ROW_Y + ListRow::DEFAULT_H + 8;

static const int16_t HINT_Y = 220;
static const int16_t RESETTING_Y = 260;

ResetScreen::ResetScreen(ScreenManager &screenManager, SoundManager &sound)
    : header("Reset"),

      warningCard(CARD_X, CARD_Y, CARD_W, CARD_H, WARNING_COLOR, true, Theme::CARD, true, 4),
      questionText(CARD_CENTER_X, CARD_Y + 14, "Reset game?", Theme::TEXT, 2, TC_DATUM),
      warningText(CARD_CENTER_X, CARD_Y + 40, "All progress will be lost.", WARNING_COLOR, 1, TC_DATUM),

      noRow(ROW_X, NO_ROW_Y),
      yesRow(ROW_X, YES_ROW_Y),

      hintText(120, HINT_Y, "Up/Down choose - Confirm OK", HINT_COLOR, 1, TC_DATUM),
      resettingText(120, RESETTING_Y, "Resetting...", WARNING_COLOR, 2, TC_DATUM),

      screenManager(screenManager),
      sound(sound),
      yesSelected(false),
      resetting(false)
{
    addElement(&header);
    setHeader(&header);

    addElement(&warningCard);
    addElement(&questionText, Theme::CARD);
    addElement(&warningText, Theme::CARD);

    noRow.setTitle("No");
    noRow.setSubtitle("Keep playing");
    yesRow.setTitle("Yes");
    yesRow.setSubtitle("Erase the save and restart");
    yesRow.setFrameColor(WARNING_COLOR);
    yesRow.setTitleColor(WARNING_COLOR);
    addElement(&noRow);
    addElement(&yesRow);

    addElement(&hintText);
    resettingText.setVisible(false);
    addElement(&resettingText);

    refreshRows();
}

void ResetScreen::refreshRows()
{
    noRow.setState(yesSelected ? ListRow::NORMAL : ListRow::SELECTED);
    yesRow.setState(yesSelected ? ListRow::SELECTED : ListRow::NORMAL);
}

void ResetScreen::onEnter(TFT_eSPI &tft)
{
    // Every visit starts on No, whatever was selected when it was last cancelled
    yesSelected = false;
    refreshRows();
    Screen::onEnter(tft);
}

void ResetScreen::render(TFT_eSPI &tft)
{
    Screen::render(tft);

    // Only after the "Resetting..." text is on the panel, so the player sees why it blanks
    if (resetting)
    {
        eraseSave();
        ESP.restart();
    }
}

void ResetScreen::onUpPress()
{
    if (resetting)
    {
        return;
    }
    yesSelected = !yesSelected;
    refreshRows();
    sound.playMenu();
}

void ResetScreen::onDownPress()
{
    onUpPress();
}

void ResetScreen::onConfirmPress()
{
    if (resetting)
    {
        return;
    }
    if (!yesSelected)
    {
        sound.playMenu();
        screenManager.closeModal();
        return;
    }
    resetting = true;
    resettingText.setVisible(true);
}

bool ResetScreen::onBackPress()
{
    if (!resetting)
    {
        screenManager.closeModal();
    }
    return true;
}
