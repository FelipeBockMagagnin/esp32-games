#include "GameListScreen.h"
#include <stdio.h>
#include "../play/Cart.h"
#include "../play/Crisp.h"

static const int16_t ROW_X = (DISPLAY_W - ListRow::DEFAULT_W) / 2;
static const int16_t FIRST_ROW_Y = 34;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 2;
static const int16_t HINT_Y = DISPLAY_H - 16;

// The game's own description, "[Tap] Turn\n[Hold] Fly", on one line
static void controls(const char *description, char *out, size_t size)
{
    size_t n = 0;
    for (const char *c = description; *c != '\0' && n + 1 < size; c++)
    {
        char ch = *c == '\n' ? ' ' : *c;
        if (ch == ' ' && (n == 0 || out[n - 1] == ' '))
        {
            continue;
        }
        out[n++] = ch;
    }
    out[n] = '\0';
}

GameListScreen::GameListScreen(void (*play)(uint16_t index), SoundManager &sound)
    : header("Games"),
      emptyText(DISPLAY_W / 2, 110, "No games in this build", Theme::TEXT_DIM, 1, TC_DATUM),
      hintText(DISPLAY_W / 2, HINT_Y, "", Theme::TEXT_DIM, 1, TC_DATUM),
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
    list.setCount(CART_COUNT);
    emptyText.setVisible(CART_COUNT == 0);
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
        row.setTitle(crispTitle(index));
        controls(crispDescription(index), subtitles[i], sizeof(subtitles[i]));
        row.setSubtitle(subtitles[i]);
        int hi = crispHiScore(index);
        snprintf(values[i], sizeof(values[i]), hi > 0 ? "%d" : "", hi);
        row.setValue(values[i]);
        row.setValueColor(Theme::GOLD);
        row.setState(index == list.getSelected() ? ListRow::SELECTED : ListRow::NORMAL);
    }
    char buf[40];
    snprintf(buf, sizeof(buf), "%u of %u - OK to play", (unsigned)list.getSelected() + 1, (unsigned)CART_COUNT);
    hintText.setText(CART_COUNT > 0 ? buf : "");
}

void GameListScreen::onEnter(TFT_eSPI &tft)
{
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
    if (CART_COUNT > 0)
    {
        play(list.getSelected());
    }
}

void GameListScreen::setSelected(uint16_t index)
{
    list.setCount(CART_COUNT);
    list.setSelected(index < CART_COUNT ? index : 0);
}
