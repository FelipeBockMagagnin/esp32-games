#include "SettingsScreen.h"
#include "Launcher.h"
#include "../ui/Themes.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 36;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 4;
static const int16_t HINT_Y = 306;

enum Row : uint8_t
{
    ROW_FOCUS,
    ROW_BREAK,
    ROW_LONG_BREAK,
    ROW_AUTO_START,
    ROW_SOUND,
    ROW_DIM,
    ROW_THEME,
    ROW_MENU,
    ROW_RESET
};

static const char *const SOUND_NAMES[(uint8_t)SoundMode::COUNT] = {"On", "Quiet", "Off"};
static const char *const SOUND_NOTES[(uint8_t)SoundMode::COUNT] = {"Every sound", "Only the alarms", "Silent"};
static const char *const DIM_NAMES[DIM_OPTION_COUNT] = {"1 min", "5 min", "Never"};

static void setMinutes(ListRow &row, uint8_t minutes)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u min", (unsigned)minutes);
    row.setValue(buf);
}

SettingsScreen::SettingsScreen(Settings &settings, void (*changed)(), void (*toMenu)(),
                               ScreenManager &screenManager, SoundManager &sound, Screen &resetScreen)
    : header("Settings"),
      hintText(120, HINT_Y, "Up/Down choose - Confirm change", Theme::TEXT_DIM, 1, TC_DATUM),
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
    rows[ROW_FOCUS].setTitle("Focus");
    rows[ROW_BREAK].setTitle("Break");
    rows[ROW_LONG_BREAK].setTitle("Long break");
    rows[ROW_LONG_BREAK].setSubtitle("After every 4 focuses");
    rows[ROW_AUTO_START].setTitle("Auto start");
    rows[ROW_SOUND].setTitle("Sound");
    rows[ROW_DIM].setTitle("Screen dim");
    rows[ROW_DIM].setSubtitle("After no button is pressed");
    rows[ROW_THEME].setTitle("Theme");
    rows[ROW_THEME].setIconPixels(swatch, SWATCH_SIZE, SWATCH_SIZE);
    rows[ROW_MENU].setTitle("Game menu");
    rows[ROW_MENU].setSubtitle("Switch apps");
    rows[ROW_RESET].setTitle("Reset stats");
    rows[ROW_RESET].setSubtitle("Erase every focus count");
    rows[ROW_RESET].setTitleColor(Theme::BAD);
    rows[ROW_RESET].setFrameColor(Theme::BAD);
    addElement(&hintText);
}

void SettingsScreen::refreshRows()
{
    setMinutes(rows[ROW_FOCUS], FOCUS_OPTIONS[settings.focusMin]);
    setMinutes(rows[ROW_BREAK], BREAK_OPTIONS[settings.breakMin]);
    setMinutes(rows[ROW_LONG_BREAK], LONG_BREAK_OPTIONS[settings.longBreakMin]);
    rows[ROW_AUTO_START].setValue(settings.autoStart ? "On" : "Off");
    rows[ROW_AUTO_START].setSubtitle(settings.autoStart ? "Next phase starts by itself" : "Each phase waits for you");
    rows[ROW_SOUND].setValue(SOUND_NAMES[settings.sound]);
    rows[ROW_SOUND].setSubtitle(SOUND_NOTES[settings.sound]);
    rows[ROW_DIM].setValue(DIM_NAMES[settings.dimAfter]);
    const ThemeDef &theme = THEMES[Theme::getActive()];
    rows[ROW_THEME].setValue(theme.name);
    rows[ROW_THEME].setSubtitle(theme.note);
    paintSwatch();
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
}

// 2x2 squares with a transparent gap, so the row's fill shows between them
void SettingsScreen::paintSwatch()
{
    static const int16_t CELL = 7;
    const Color colors[4] = {Theme::BAD, Theme::GOOD, Theme::INFO, Theme::GOLD};
    for (int16_t row = 0; row < SWATCH_SIZE; row++)
    {
        for (int16_t col = 0; col < SWATCH_SIZE; col++)
        {
            int16_t cx = col / (CELL + 1), cy = row / (CELL + 1);
            bool gap = col % (CELL + 1) == CELL || row % (CELL + 1) == CELL;
            uint16_t c = gap ? 0 : (uint16_t)colors[cy * 2 + cx];
            swatch[row * SWATCH_SIZE + col] = (uint16_t)((c >> 8) | (c << 8));
        }
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
    case ROW_FOCUS:
        settings.focusMin = (settings.focusMin + 1) % FOCUS_OPTION_COUNT;
        break;
    case ROW_BREAK:
        settings.breakMin = (settings.breakMin + 1) % BREAK_OPTION_COUNT;
        break;
    case ROW_LONG_BREAK:
        settings.longBreakMin = (settings.longBreakMin + 1) % LONG_BREAK_OPTION_COUNT;
        break;
    case ROW_AUTO_START:
        settings.autoStart = !settings.autoStart;
        break;
    case ROW_SOUND:
        settings.sound = (settings.sound + 1) % (uint8_t)SoundMode::COUNT;
        break;
    case ROW_DIM:
        settings.dimAfter = (settings.dimAfter + 1) % DIM_OPTION_COUNT;
        break;
    case ROW_THEME:
        settings.theme = (settings.theme + 1) % THEME_COUNT;
        changed();
        refreshRows();
        sound.playMenu();
        screenManager.repaint();
        return;
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

bool SettingsScreen::onBackPress()
{
    screenManager.closeModal();
    return true;
}
