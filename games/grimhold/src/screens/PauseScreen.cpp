#include "PauseScreen.h"
#include "../render/Art.h"

static const int16_t ROW_X = (DISPLAY_W - ListRow::DEFAULT_W) / 2;
static const int16_t FIRST_ROW_Y = 34;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 3;
static const int16_t STATUS_Y = 214;

enum Row : uint8_t
{
    ROW_RESUME,
    ROW_INVENTORY,
    ROW_HEALTH,
    ROW_MANA,
    ROW_SETTINGS,
    ROW_ABANDON
};

PauseScreen::PauseScreen(Run &run, ScreenManager &screenManager, SoundManager &sound, Screen &playScreen,
                         Screen &inventoryScreen, Screen &settingsScreen, Screen &abandonScreen)
    : header("Paused"),
      statusText(DISPLAY_W / 2, STATUS_Y, "", Theme::TEXT_2, 1, TC_DATUM),
      run(run),
      screenManager(screenManager),
      sound(sound),
      playScreen(playScreen),
      inventoryScreen(inventoryScreen),
      settingsScreen(settingsScreen),
      abandonScreen(abandonScreen)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    rows[ROW_RESUME].setTitle("Resume");
    rows[ROW_INVENTORY].setTitle("Inventory");
    rows[ROW_HEALTH].setTitle("Health potion");
    rows[ROW_HEALTH].setIconPixels(ITEM_ICONS[ITEM_POTION_FIRST], 16, 16);
    rows[ROW_MANA].setTitle("Mana potion");
    rows[ROW_MANA].setIconPixels(ITEM_ICONS[ITEM_POTION_FIRST + 1], 16, 16);
    rows[ROW_SETTINGS].setTitle("Settings");
    rows[ROW_ABANDON].setTitle("Abandon run");
    rows[ROW_ABANDON].setTitleColor(Theme::BAD);
    rows[ROW_ABANDON].setFrameColor(Theme::BAD);
    addElement(&statusText);
}

void PauseScreen::refreshRows()
{
    char text[48];
    snprintf(text, sizeof(text), "x%u", run.inventory.hpPotions);
    rows[ROW_HEALTH].setValue(text);
    snprintf(text, sizeof(text), "x%u", run.inventory.mpPotions);
    rows[ROW_MANA].setValue(text);
    snprintf(text, sizeof(text), "Floor %u   HP %d/%u   MP %u/%u", run.getFloor(), run.getHp(), run.maxHp(),
             run.getMana(), run.maxMana());
    statusText.setText(text);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setState(i == selected ? ListRow::SELECTED : ListRow::NORMAL);
    }
    snprintf(text, sizeof(text), "%u/%u", run.inventory.bagCount(), BAG_SIZE);
    rows[ROW_INVENTORY].setValue(text);
}

void PauseScreen::onEnter(TFT_eSPI &tft)
{
    refreshRows();
    Screen::onEnter(tft);
}

void PauseScreen::onUpPress()
{
    selected = (selected + ROW_COUNT - 1) % ROW_COUNT;
    refreshRows();
    sound.playMenu();
}

void PauseScreen::onDownPress()
{
    selected = (selected + 1) % ROW_COUNT;
    refreshRows();
    sound.playMenu();
}

void PauseScreen::onConfirmPress()
{
    switch (selected)
    {
    case ROW_RESUME:
        sound.playMenu();
        selected = 0;
        screenManager.replaceModal(&playScreen);
        return;
    case ROW_INVENTORY:
        sound.playMenu();
        screenManager.replaceModal(&inventoryScreen);
        return;
    case ROW_HEALTH:
    case ROW_MANA:
        if (run.usePotion(selected == ROW_HEALTH))
        {
            sound.playDrink();
        }
        else
        {
            sound.playError();
        }
        refreshRows();
        return;
    case ROW_SETTINGS:
        sound.playMenu();
        screenManager.replaceModal(&settingsScreen);
        return;
    case ROW_ABANDON:
        sound.playMenu();
        screenManager.replaceModal(&abandonScreen);
        return;
    }
}

bool PauseScreen::onBackPress()
{
    sound.playMenu();
    selected = 0;
    screenManager.replaceModal(&playScreen);
    return true;
}
