#include "SettingsScreen.h"
#include "Launcher.h"
#include "../ui/Themes.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 44;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;

enum Row : uint8_t
{
    ROW_THEME,
    ROW_MODE,
    ROW_DIM,
    ROW_CREDITS,
    ROW_MENU
};

static const char *const DIM_NAMES[DIM_OPTION_COUNT] = {"1 min", "5 min", "Nunca"};

SettingsScreen::SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(), Transport &transport,
                               ScreenManager &screenManager, Screen &credits)
    : header("Ajustes"),
      settings(settings),
      changed(changed),
      toMenu(toMenu),
      transport(transport),
      screenManager(screenManager),
      credits(credits)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    rows[ROW_THEME].setTitle("Tema");
    rows[ROW_MODE].setTitle("Ao acabar");
    rows[ROW_DIM].setTitle("Escurecer");
    rows[ROW_CREDITS].setTitle("Creditos");
    rows[ROW_MENU].setTitle("Menu jogos");
}

void SettingsScreen::refreshRows()
{
    rows[ROW_THEME].setValue(THEMES[Theme::getActive()].name);
    rows[ROW_MODE].setValue(modeName(transport.playlist.mode));
    rows[ROW_DIM].setValue(DIM_NAMES[settings.dimAfter < DIM_OPTION_COUNT ? settings.dimAfter : DIM_DEFAULT]);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
}

void SettingsScreen::onEnter(TFT_eSPI &tft)
{
    menuShown = launcherInstalled();
    rows[ROW_MENU].setVisible(menuShown);
    if (selected == ROW_MENU && !menuShown)
    {
        selected = 0;
    }
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
    case ROW_THEME:
        settings.theme = (Theme::getActive() + 1) % THEME_COUNT;
        changed();
        refreshRows();
        screenManager.repaint();
        return;
    case ROW_MODE:
        transport.cycleMode();
        break;
    case ROW_DIM:
        settings.dimAfter = (settings.dimAfter + 1) % DIM_OPTION_COUNT;
        changed();
        break;
    case ROW_CREDITS:
        screenManager.replaceModal(&credits);
        return;
    case ROW_MENU:
        toMenu();
        return;
    }
    refreshRows();
}

bool SettingsScreen::onBackPress()
{
    screenManager.closeModal();
    return true;
}

bool SettingsScreen::onRightPress()
{
    return true;
}
