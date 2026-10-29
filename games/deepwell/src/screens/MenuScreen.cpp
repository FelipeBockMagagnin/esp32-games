#include "MenuScreen.h"

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 40;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 6;
static const int16_t SHEET_X = 10;
static const int16_t SHEET_Y = 176;
static const int16_t SHEET_PITCH = 13;
static const int16_t HINT_Y = 300;

MenuScreen::MenuScreen(const Run &run, ScreenManager &screenManager, SoundManager &sound, Screen &settingsScreen)
    : header("Actions"),
      hintText(120, HINT_Y, "Confirm choose - Left back", Theme::TEXT_DIM, 1, TC_DATUM),
      run(run),
      screenManager(screenManager),
      sound(sound),
      settingsScreen(settingsScreen),
      dungeon(nullptr),
      optionCount(0),
      selected(0)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < OPTION_COUNT; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    for (uint8_t i = 0; i < SHEET_LINES; i++)
    {
        sheet[i].setPosition(SHEET_X, SHEET_Y + i * SHEET_PITCH);
        sheet[i].setColor(i == 0 ? Theme::TEXT : Theme::TEXT_2);
        addElement(&sheet[i]);
    }
    addElement(&hintText);
}

void MenuScreen::build()
{
    optionCount = 0;
    if (run.onStairs())
    {
        options[optionCount++] = OPTION_DESCEND;
    }
    options[optionCount++] = OPTION_DRINK;
    options[optionCount++] = OPTION_WAIT;
    options[optionCount++] = OPTION_SETTINGS;

    char buf[40];
    for (uint8_t i = 0; i < OPTION_COUNT; i++)
    {
        ListRow &row = rows[i];
        row.setVisible(i < optionCount);
        if (i >= optionCount)
        {
            continue;
        }
        switch (options[i])
        {
        case OPTION_DESCEND:
            row.setTitle("Descend");
            snprintf(buf, sizeof(buf), "Down to floor %u", (unsigned)run.getFloor() + 1);
            row.setSubtitle(buf);
            row.setValue("");
            break;
        case OPTION_DRINK:
            row.setTitle("Drink potion");
            row.setSubtitle(run.getHp() < run.getMaxHp() ? "Heals half your health" : "Already at full health");
            snprintf(buf, sizeof(buf), "%u left", (unsigned)run.getPotions());
            row.setValue(buf);
            break;
        case OPTION_WAIT:
            row.setTitle("Wait");
            row.setSubtitle("Let a turn pass");
            row.setValue("");
            break;
        default:
            row.setTitle("Settings");
            row.setSubtitle("Sound, screen, theme, reset");
            row.setValue("");
            break;
        }
    }

    snprintf(buf, sizeof(buf), "Floor %u  Level %u  XP %u/%u", (unsigned)run.getFloor(), (unsigned)run.getLevel(),
             (unsigned)run.getXp(), (unsigned)xpForLevel(run.getLevel()));
    sheet[0].setText(buf);
    snprintf(buf, sizeof(buf), "Attack %d  Defense %d", run.getAttack(), run.getDefense());
    sheet[1].setText(buf);
    snprintf(buf, sizeof(buf), "Gold %lu  Kills %u", (unsigned long)run.getGold(), (unsigned)run.getKills());
    sheet[2].setText(buf);
    snprintf(buf, sizeof(buf), "Score %lu", (unsigned long)run.getScore());
    sheet[3].setText(buf);
    const char *weapon = run.getWeapon() > 0 ? ITEMS[run.getWeapon() - 1].name : "Fists";
    const char *armor = run.getArmor() > 0 ? ITEMS[run.getArmor() - 1].name : "Rags";
    snprintf(buf, sizeof(buf), "%s, %s", weapon, armor);
    sheet[4].setText(buf);
}

void MenuScreen::refreshRows()
{
    for (uint8_t i = 0; i < optionCount; i++)
    {
        bool useless = options[i] == OPTION_DRINK && (run.getPotions() == 0 || run.getHp() >= run.getMaxHp());
        rows[i].setState(i == selected ? ListRow::SELECTED : (useless ? ListRow::DIMMED : ListRow::NORMAL));
    }
}

void MenuScreen::onEnter(TFT_eSPI &tft)
{
    selected = 0;
    build();
    refreshRows();
    Screen::onEnter(tft);
}

void MenuScreen::onUpPress()
{
    selected = selected > 0 ? selected - 1 : optionCount - 1;
    refreshRows();
    sound.playMenu();
}

void MenuScreen::onDownPress()
{
    selected = (selected + 1) % optionCount;
    refreshRows();
    sound.playMenu();
}

void MenuScreen::onConfirmPress()
{
    Option option = options[selected];
    if (option == OPTION_SETTINGS)
    {
        sound.playMenu();
        screenManager.replaceModal(&settingsScreen);
        return;
    }
    // Closed first, so a death's game-over modal can open over the dungeon
    screenManager.closeModal();
    if (dungeon == nullptr)
    {
        return;
    }
    dungeon->act(option == OPTION_DESCEND ? Action::DESCEND : (option == OPTION_DRINK ? Action::DRINK : Action::WAIT));
}

bool MenuScreen::onBackPress()
{
    sound.playMenu();
    screenManager.closeModal();
    return true;
}
