#include "SettingsScreen.h"
#include "Launcher.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 44;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;

enum Row : uint8_t
{
    ROW_SOUND,
    ROW_DIM,
    ROW_MENU,
    ROW_RESET
};

static const char *const SOUND_NAMES[(uint8_t)SoundMode::COUNT] = {"On", "Quiet", "Off"};
static const char *const DIM_NAMES[DIM_OPTION_COUNT] = {"1 min", "5 min", "Never"};

SettingsScreen::SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(),
                               ScreenManager &screenManager, SoundManager &sound, Screen &resetScreen)
    : header("Settings"),
      settings(settings),
      changed(changed),
      toMenu(toMenu),
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
    rows[ROW_MENU].setTitle("Game menu");
    rows[ROW_RESET].setTitle("Reset");
    rows[ROW_RESET].setTitleColor(Theme::BAD);
    rows[ROW_RESET].setFrameColor(Theme::BAD);
}

void SettingsScreen::refreshRows()
{
    rows[ROW_SOUND].setValue(SOUND_NAMES[settings.sound < (uint8_t)SoundMode::COUNT ? settings.sound : 0]);
    rows[ROW_DIM].setValue(DIM_NAMES[settings.dimAfter < DIM_OPTION_COUNT ? settings.dimAfter : DIM_DEFAULT]);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
}

void SettingsScreen::onEnter(TFT_eSPI &tft)
{
    selected = 0;
    menuShown = launcherInstalled();
    rows[ROW_MENU].setVisible(menuShown);
    rows[ROW_RESET].setPosition(ROW_X, FIRST_ROW_Y + (menuShown ? ROW_RESET : ROW_MENU) * ROW_PITCH);
    refreshRows();
    Screen::onEnter(tft);
}

void SettingsScreen::step(int8_t delta)
{
    do
    {
        selected = (selected + ROW_COUNT + delta) % ROW_COUNT;
    } while (selected == ROW_MENU && !menuShown);
    refreshRows();
    sound.playMenu();
}

void SettingsScreen::onUpPress()
{
    step(-1);
}

void SettingsScreen::onDownPress()
{
    step(1);
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
    case ROW_MENU:
        sound.playMenu();
        toMenu();
        return;
    case ROW_RESET:
        sound.playMenu();
        screenManager.replaceModal(&resetScreen);
        return;
    }
    changed();
    refreshRows();
    sound.playMenu();
}
