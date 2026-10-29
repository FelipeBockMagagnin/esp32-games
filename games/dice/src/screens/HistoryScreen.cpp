#include "HistoryScreen.h"
#include <stdio.h>
#include <string.h>

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 34;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 2;
static const int16_t HINT_Y = 300;
static const int16_t ICON_SIZE = 16;
// What fits beside the icon, in the row's size-1 font
static const size_t SUBTITLE_CHARS = 32;

static const unsigned char DIE_ICON[] = {
    0x3F, 0xFC, 0x40, 0x02, 0x80, 0x01, 0x98, 0x19, 0x98, 0x19, 0x80, 0x01, 0x80, 0x01, 0x81, 0x81,
    0x81, 0x81, 0x80, 0x01, 0x80, 0x01, 0x98, 0x19, 0x98, 0x19, 0x80, 0x01, 0x40, 0x02, 0x3F, 0xFC};
static const unsigned char BALL_ICON[] = {
    0x07, 0xE0, 0x18, 0x18, 0x20, 0x04, 0x44, 0x22, 0x44, 0x22, 0x9F, 0xF9, 0x84, 0x21, 0x84, 0x21,
    0x84, 0x21, 0x84, 0x21, 0x9F, 0xF9, 0x44, 0x22, 0x44, 0x22, 0x20, 0x04, 0x18, 0x18, 0x07, 0xE0};
static const unsigned char WHEEL_ICON[] = {
    0x03, 0xC0, 0x01, 0x80, 0x1E, 0x78, 0x21, 0x84, 0x41, 0x82, 0x61, 0x86, 0x99, 0x99, 0x87, 0xE1,
    0x87, 0xE1, 0x99, 0x99, 0x61, 0x86, 0x41, 0x82, 0x21, 0x84, 0x1E, 0x78, 0x07, 0xE0, 0x00, 0x00};

HistoryScreen::HistoryScreen(const History &history, SoundManager &sound)
    : header("History"),
      emptyText(120, 140, "Nothing rolled yet", Theme::TEXT_DIM, 2, TC_DATUM),
      hintText(120, HINT_Y, "", Theme::TEXT_DIM, 1, TC_DATUM),
      history(history),
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

static void formatAge(unsigned long ms, char *buf, size_t len)
{
    unsigned long minutes = ms / 60000;
    if (minutes == 0)
    {
        snprintf(buf, len, "now");
    }
    else if (minutes < 60)
    {
        snprintf(buf, len, "%lu min", minutes);
    }
    else
    {
        snprintf(buf, len, "%lu h", minutes / 60);
    }
}

void HistoryScreen::refresh(unsigned long now)
{
    if (history.getVersion() != shownVersion)
    {
        shownVersion = history.getVersion();
        list.reset();
    }
    list.setCount(history.count());
    emptyText.setVisible(history.count() == 0);

    char buf[48];
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
        const HistoryEntry &entry = history.get((uint8_t)index);
        const unsigned char *icon = entry.tool == Tool::ROLL ? DIE_ICON : (entry.tool == Tool::DRAW ? BALL_ICON : WHEEL_ICON);
        row.setIconBitmap(icon, ICON_SIZE, ICON_SIZE, Theme::TEXT_2);
        snprintf(buf, sizeof(buf), "%d", (int)entry.result);
        row.setTitle(buf);
        Color mark = entry.mark == Mark::GOOD ? Theme::GOOD : (entry.mark == Mark::BAD ? Theme::BAD : Theme::TEXT);
        row.setTitleColor(mark);
        if (entry.detail[0] != '\0')
        {
            snprintf(buf, sizeof(buf), "%.*s", (int)SUBTITLE_CHARS, entry.what);
            size_t used = strlen(buf);
            snprintf(buf + used, sizeof(buf) - used, ": %.*s", (int)(SUBTITLE_CHARS - used), entry.detail);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%.*s", (int)SUBTITLE_CHARS, entry.what);
        }
        row.setSubtitle(buf);
        formatAge(now - entry.at, buf, sizeof(buf));
        row.setValue(buf);
        row.setState(index == list.getSelected() ? ListRow::SELECTED : ListRow::NORMAL);
    }

    if (history.count() == 0)
    {
        hintText.setText("Roll, draw or spin to fill it");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%u of %u - kept until power off", (unsigned)list.getSelected() + 1,
                 (unsigned)history.count());
        hintText.setText(buf);
    }
}

void HistoryScreen::onEnter(TFT_eSPI &tft)
{
    list.reset();
    refresh(millis());
    Screen::onEnter(tft);
}

void HistoryScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    refresh(now);
}

void HistoryScreen::onUpPress()
{
    if (history.count() == 0)
    {
        return;
    }
    list.previous();
    forceRefresh();
    sound.playMenu();
}

void HistoryScreen::onDownPress()
{
    if (history.count() == 0)
    {
        return;
    }
    list.next();
    forceRefresh();
    sound.playMenu();
}

bool HistoryScreen::onRepeat(Key key)
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
