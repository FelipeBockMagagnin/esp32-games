#include "TitleScreen.h"

static const int16_t LEFT = 16;
static const int16_t MENU_Y = 150;
static const int16_t MENU_PITCH = 24;
static const char *const MENU_LABELS[] = {"START", "SETTINGS"};
// The idle frame and the one lifting the empty mug
static const uint8_t HERO_FRAMES[] = {0, 16};
static const unsigned long HERO_FRAME_MS = 700;

TitleScreen::TitleScreen(ScreenManager &screenManager, SoundManager &sound, const SaveBlob &records,
                         Screen &settingsScreen, void (*start)(uint8_t level))
    : screenManager(screenManager),
      sound(sound),
      records(records),
      settingsScreen(settingsScreen),
      start(start),
      selected(0),
      titleText(LEFT, 22, "DECAF", Theme::GOLD, 5),
      infernoText(LEFT, 68, "INFERNO", Theme::BAD, 4),
      subtitleText{Text(LEFT, 110, "The coffee machine broke", Theme::TEXT_2),
                   Text(LEFT, 122, "and HR won't buy another.", Theme::TEXT_2)},
      menuRows{Text(LEFT, MENU_Y, "", Theme::TEXT, 2), Text(LEFT, MENU_Y + MENU_PITCH, "", Theme::TEXT, 2)},
      bestText(LEFT, 220, "", Theme::TEXT_DIM),
      hero(DISPLAY_W - 4 * 32 - 8, 92, sheet_player, 4, Theme::BG)
{
    addElement(&titleText);
    addElement(&infernoText);
    for (Text &line : subtitleText)
    {
        addElement(&line);
    }
    for (Text &row : menuRows)
    {
        addElement(&row);
    }
    addElement(&bestText);
    addElement(&hero);
}

void TitleScreen::refreshMenu()
{
    char text[16];
    for (uint8_t i = 0; i < MENU_ROWS; i++)
    {
        snprintf(text, sizeof(text), "%s %s", i == selected ? ">" : " ", MENU_LABELS[i]);
        menuRows[i].setText(text);
        menuRows[i].setColor(i == selected ? Theme::SELECT : Theme::TEXT_DIM);
    }
}

void TitleScreen::onEnter(TFT_eSPI &tft)
{
    selected = 0;
    refreshMenu();
    const FloorRecord &best = records.floors[0];
    if (best.clears > 0)
    {
        uint32_t tenths = best.bestTicks / 6;
        char text[48];
        snprintf(text, sizeof(text), "B1 best %lu:%02lu.%lu - fewest deaths %u", (unsigned long)(tenths / 600),
                 (unsigned long)(tenths / 10 % 60), (unsigned long)(tenths % 10), (unsigned)best.fewestDeaths);
        bestText.setText(text);
    }
    else
    {
        bestText.setText("Confirm to start");
    }
    Screen::onEnter(tft);
}

void TitleScreen::update(unsigned long now)
{
    hero.setFrame(HERO_FRAMES[(now / HERO_FRAME_MS) % 2]);
}

void TitleScreen::onConfirmPress()
{
    sound.playMenu();
    if (selected == 0)
    {
        start(0);
    }
    else
    {
        screenManager.openModal(&settingsScreen);
    }
}

void TitleScreen::onUpPress()
{
    selected = selected > 0 ? selected - 1 : MENU_ROWS - 1;
    refreshMenu();
    sound.playMenu();
}

void TitleScreen::onDownPress()
{
    selected = (selected + 1) % MENU_ROWS;
    refreshMenu();
    sound.playMenu();
}
