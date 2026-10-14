#include "SettingsScreen.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 44;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;
static const int16_t HINT_Y = 220;

enum Row : uint8_t
{
    ROW_SOUND,
    ROW_DIM,
    ROW_RESET
};

static const char *const SOUND_NAMES[(uint8_t)SoundMode::COUNT] = {"On", "Quiet", "Off"};
static const char *const SOUND_NOTES[(uint8_t)SoundMode::COUNT] = {
    "Every sound", "Awards, veins, new items", "Silent"};
static const char *const DIM_NAMES[DIM_OPTION_COUNT] = {"1 min", "5 min", "Never"};

SettingsScreen::SettingsScreen(Settings &settings, void (*changed)(), ScreenManager &screenManager,
                               SoundManager &sound, Screen &resetScreen)
    : header("Settings"),
      hintText(120, HINT_Y, "Up/Down choose - Confirm change", Theme::TEXT_DIM, 1, TC_DATUM),
      settings(settings),
      changed(changed),
      screenManager(screenManager),
      sound(sound),
      resetScreen(resetScreen),
      selected(0)
{
    addElement(&header);
    setHeader(&header);

    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    rows[ROW_SOUND].setTitle("Sound");
    rows[ROW_DIM].setTitle("Screen dim");
    rows[ROW_DIM].setSubtitle("After no button is pressed");
    rows[ROW_RESET].setTitle("Reset game");
    rows[ROW_RESET].setSubtitle("Erase all progress");
    rows[ROW_RESET].setTitleColor(Theme::BAD);
    rows[ROW_RESET].setFrameColor(Theme::BAD);
    addElement(&hintText);
}

void SettingsScreen::refreshRows()
{
    uint8_t mode = settings.sound < (uint8_t)SoundMode::COUNT ? settings.sound : 0;
    rows[ROW_SOUND].setValue(SOUND_NAMES[mode]);
    rows[ROW_SOUND].setSubtitle(SOUND_NOTES[mode]);
    rows[ROW_DIM].setValue(DIM_NAMES[settings.dimAfter < DIM_OPTION_COUNT ? settings.dimAfter : DIM_DEFAULT]);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
}

void SettingsScreen::onEnter(TFT_eSPI &tft)
{
    selected = 0;
    refreshRows();
    Screen::onEnter(tft);
}

void SettingsScreen::onUpPress()
{
    selected = selected > 0 ? selected - 1 : ROW_COUNT - 1;
    refreshRows();
    sound.playMenu();
}

void SettingsScreen::onDownPress()
{
    selected = (selected + 1) % ROW_COUNT;
    refreshRows();
    sound.playMenu();
}

void SettingsScreen::onConfirmPress()
{
    switch (selected)
    {
    case ROW_SOUND:
        settings.sound = (settings.sound + 1) % (uint8_t)SoundMode::COUNT;
        break;
    case ROW_DIM:
        settings.dimAfter = (settings.dimAfter + 1) % DIM_OPTION_COUNT;
        break;
    case ROW_RESET:
        sound.playMenu();
        screenManager.replaceModal(&resetScreen);
        return;
    }
    changed();
    refreshRows();
    sound.playMenu();
}

bool SettingsScreen::onBackPress()
{
    screenManager.closeModal();
    return true;
}
