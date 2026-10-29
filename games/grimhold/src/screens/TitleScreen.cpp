#include "TitleScreen.h"

static const int16_t LEFT = 16;
static const int16_t MENU_Y = 104;
static const int16_t MENU_PITCH = 26;
static const char *const LABELS[] = {"Continue", "New run", "Settings"};

TitleScreen::TitleScreen(SoundManager &sound, const SaveBlob &save, const Hooks &hooks)
    : sound(sound),
      save(save),
      hooks(hooks),
      titleText(LEFT, 22, "GRIMHOLD", Theme::BAD, 4),
      taglineText(LEFT, 62, "Descend. Fall. Descend again.", Theme::TEXT_2),
      menuRows{Text(LEFT, MENU_Y, "", Theme::TEXT, 2), Text(LEFT, MENU_Y + MENU_PITCH, "", Theme::TEXT, 2),
               Text(LEFT, MENU_Y + 2 * MENU_PITCH, "", Theme::TEXT, 2)},
      recordText(LEFT, 200, "", Theme::TEXT_DIM),
      hintText(LEFT, 216, "Up/Down choose - Confirm", Theme::TEXT_DIM),
      lich(DISPLAY_W - 2 * 64 - 4, 72, &ENEMY_ART[ART_ENEMY_LICH], 2)
{
    addElement(&titleText);
    addElement(&taglineText);
    for (Text &row : menuRows)
    {
        addElement(&row);
    }
    addElement(&recordText);
    addElement(&hintText);
    addElement(&lich);
}

void TitleScreen::refreshMenu()
{
    char text[24];
    for (uint8_t i = 0; i < MENU_ROWS; i++)
    {
        snprintf(text, sizeof(text), "%s %s", i == selected ? ">" : " ", LABELS[i]);
        if (i == CONTINUE)
        {
            snprintf(text + strlen(text), sizeof(text) - strlen(text), " B%u", save.run.floor);
        }
        menuRows[i].setText(text);
        menuRows[i].setColor(i == selected ? Theme::SELECT : Theme::TEXT_DIM);
    }
    menuRows[CONTINUE].setVisible(hasRun());
}

void TitleScreen::onEnter(TFT_eSPI &tft)
{
    selected = hasRun() ? CONTINUE : NEW_RUN;
    refreshMenu();
    const Records &r = save.records;
    char text[56];
    if (r.runs == 0)
    {
        recordText.setText("No one has gone down yet");
    }
    else
    {
        snprintf(text, sizeof(text), "Deepest B%u - most kills %u - runs %u", r.bestFloor, r.bestKills, r.runs);
        recordText.setText(text);
    }
    Screen::onEnter(tft);
}

void TitleScreen::step(int8_t delta)
{
    do
    {
        selected = (selected + MENU_ROWS + delta) % MENU_ROWS;
    } while (selected == CONTINUE && !hasRun());
    refreshMenu();
    sound.playMenu();
}

void TitleScreen::onUpPress()
{
    step(-1);
}

void TitleScreen::onDownPress()
{
    step(1);
}

void TitleScreen::onConfirmPress()
{
    sound.playMenu();
    switch (selected)
    {
    case CONTINUE:
        hooks.continueRun();
        break;
    case NEW_RUN:
        hooks.newRun();
        break;
    default:
        hooks.settings();
        break;
    }
}
