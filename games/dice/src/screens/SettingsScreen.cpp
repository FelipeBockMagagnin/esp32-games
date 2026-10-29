#include "SettingsScreen.h"
#include "Launcher.h"
#include "../ui/Themes.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 44;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;
static const int16_t HINT_Y = 220;

enum Row : uint8_t
{
    ROW_SOUND,
    ROW_DIM,
    ROW_THEME,
    ROW_MENU,
    ROW_RESET
};

static const char *const SOUND_NAMES[(uint8_t)SoundMode::COUNT] = {"On", "Quiet", "Off"};
static const char *const SOUND_NOTES[(uint8_t)SoundMode::COUNT] = {
    "Every sound", "Results only, no clicks", "Silent"};
static const char *const DIM_NAMES[DIM_OPTION_COUNT] = {"1 min", "5 min", "Never"};

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
    rows[ROW_SOUND].setTitle("Sound");
    rows[ROW_DIM].setTitle("Screen dim");
    rows[ROW_DIM].setSubtitle("After no button is pressed");
    rows[ROW_THEME].setTitle("Theme");
    rows[ROW_THEME].setIconPixels(swatch, SWATCH_SIZE, SWATCH_SIZE);
    rows[ROW_MENU].setTitle("Game menu");
    rows[ROW_MENU].setSubtitle("Switch games");
    rows[ROW_RESET].setTitle("Reset");
    rows[ROW_RESET].setSubtitle("Forget the dice, ranges and wheel");
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
    const ThemeDef &theme = THEMES[Theme::getActive()];
    rows[ROW_THEME].setValue(theme.name);
    rows[ROW_THEME].setSubtitle(theme.note);
    char count[8];
    snprintf(count, sizeof(count), "%u/%u", (unsigned)Theme::getActive() + 1, (unsigned)THEME_COUNT);
    rows[ROW_THEME].setValueSub(count);
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
    const Color colors[4] = {Theme::GOLD, Theme::SELECT, Theme::GOOD, Theme::INFO};
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
