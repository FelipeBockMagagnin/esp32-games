#include "GameListScreen.h"
#include <stdio.h>
#include "../play/Sketch.h"
#include "../save/Saves.h"

static const int16_t ROW_X = (DISPLAY_W - ListRow::DEFAULT_W) / 2;
static const int16_t FIRST_ROW_Y = 34;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 2;
static const int16_t HINT_Y = DISPLAY_H - 16;

GameListScreen::GameListScreen(const char *nameSpace, void (*play)(uint16_t index), SoundManager &sound)
    : header("Games"),
      emptyText(DISPLAY_W / 2, 110, "No games in this build", Theme::TEXT_DIM, 1, TC_DATUM),
      hintText(DISPLAY_W / 2, HINT_Y, "", Theme::TEXT_DIM, 1, TC_DATUM),
      nameSpace(nameSpace),
      play(play),
      sound(sound),
      list(ROWS)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    addElement(&emptyText);
    addElement(&hintText);
}

void GameListScreen::refresh()
{
    list.setCount(SKETCH_COUNT);
    emptyText.setVisible(SKETCH_COUNT == 0);
    uint8_t visible = list.getVisibleCount();
    for (uint8_t i = 0; i < ROWS; i++)
    {
        ListRow &row = rows[i];
        row.setVisible(i < visible);
        if (i >= visible)
        {
            continue;
        }
        uint16_t index = list.getFirstVisible() + i;
        const Sketch &e = SKETCHES[index];
        row.setTitle(e.title);
        row.setSubtitle(e.author);
        row.setValue(index < MAX_GAMES && saved[index] ? "SAVE" : "");
        row.setValueColor(Theme::GOOD);
        row.setState(index == list.getSelected() ? ListRow::SELECTED : ListRow::NORMAL);
    }
    char buf[40];
    snprintf(buf, sizeof(buf), "%u of %u - OK to play", (unsigned)list.getSelected() + 1, (unsigned)SKETCH_COUNT);
    hintText.setText(SKETCH_COUNT > 0 ? buf : "");
}

void GameListScreen::onEnter(TFT_eSPI &tft)
{
    for (uint16_t i = 0; i < SKETCH_COUNT && i < MAX_GAMES; i++)
    {
        saved[i] = Saves::exists(nameSpace, SKETCHES[i].id);
    }
    refresh();
    Screen::onEnter(tft);
}

void GameListScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    refresh();
}

void GameListScreen::onUpPress()
{
    list.previous();
    forceRefresh();
    sound.playMenu();
}

void GameListScreen::onDownPress()
{
    list.next();
    forceRefresh();
    sound.playMenu();
}

bool GameListScreen::onRepeat(Key key)
{
    if (key == Key::UP)
    {
        onUpPress();
        return true;
    }
    if (key == Key::DOWN)
    {
        onDownPress();
        return true;
    }
    return false;
}

void GameListScreen::onConfirmPress()
{
    if (SKETCH_COUNT > 0)
    {
        play(list.getSelected());
    }
}

void GameListScreen::setSelected(uint16_t index)
{
    list.setCount(SKETCH_COUNT);
    list.setSelected(index < SKETCH_COUNT ? index : 0);
}
