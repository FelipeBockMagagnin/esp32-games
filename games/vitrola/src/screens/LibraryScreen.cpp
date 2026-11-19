#include "LibraryScreen.h"
#include <stdio.h>

static const int16_t ROW_X = 5;
static const int16_t FIRST_ROW_Y = 36;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 8;

static String duration(uint32_t frames)
{
    char buf[12];
    uint32_t s = frames / FRAME_RATE;
    snprintf(buf, sizeof(buf), "%lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60));
    return String(buf);
}

LibraryScreen::LibraryScreen(Transport &transport, ScreenManager &screenManager, Screen &settings)
    : header("Discos"), transport(transport), screenManager(screenManager), settings(settings)
{
    addElement(&header);
    setHeader(&header);
    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
}

uint8_t LibraryScreen::count() const
{
    return album < 0 ? ALBUM_COUNT + 1 : ALBUMS[album].count;
}

void LibraryScreen::onEnter(TFT_eSPI &tft)
{
    album = -1;
    selected = transport.playlist.album();
    first = 0;
    refresh();
    Screen::onEnter(tft);
}

void LibraryScreen::refresh()
{
    uint8_t n = count();
    if (selected < first)
    {
        first = selected;
    }
    else if (selected >= first + ROWS)
    {
        first = selected - ROWS + 1;
    }
    header.setTitle(album < 0 ? String("Discos") : String(ALBUMS[album].name));
    for (uint8_t i = 0; i < ROWS; i++)
    {
        uint8_t k = first + i;
        ListRow &row = rows[i];
        row.setVisible(k < n);
        if (k >= n)
        {
            continue;
        }
        bool owned = false;
        if (album < 0 && k == ALBUM_COUNT)
        {
            row.setTitle("Ajustes");
            row.setSubtitle("Tema, tela, creditos");
            row.setValue("");
            row.setValueSub("");
        }
        else if (album < 0)
        {
            const Album &a = ALBUMS[k];
            char sub[48];
            snprintf(sub, sizeof(sub), "%u faixas - %s", a.count, duration(a.frames).c_str());
            row.setTitle(a.name);
            row.setSubtitle(sub);
            row.setValue("");
            row.setValueSub("");
            owned = k == transport.playlist.album();
        }
        else
        {
            uint16_t t = ALBUMS[album].first + k;
            char sub[48];
            snprintf(sub, sizeof(sub), "%s - %s", TRACKS[t].author, duration(TRACKS[t].frames).c_str());
            row.setTitle(TRACKS[t].shelf);
            row.setSubtitle(sub);
            row.setValue("");
            row.setValueSub("");
            owned = t == transport.playlist.current && transport.player.isLoaded();
        }
        row.setState(k == selected ? ListRow::SELECTED : owned ? ListRow::OWNED : ListRow::NORMAL);
    }
}

void LibraryScreen::move(int8_t delta)
{
    uint8_t n = count();
    selected = (uint8_t)((selected + n + delta) % n);
    refresh();
}

void LibraryScreen::onUpPress()
{
    move(-1);
}

void LibraryScreen::onDownPress()
{
    move(1);
}

bool LibraryScreen::onRepeat(Key key)
{
    if (key == Key::UP || key == Key::DOWN)
    {
        move(key == Key::UP ? -1 : 1);
        return true;
    }
    return false;
}

void LibraryScreen::onConfirmPress()
{
    if (album < 0 && selected == ALBUM_COUNT)
    {
        screenManager.replaceModal(&settings);
        return;
    }
    if (album < 0)
    {
        album = selected;
        uint16_t current = transport.playlist.current;
        const Album &a = ALBUMS[album];
        selected = current >= a.first && current < a.first + a.count ? current - a.first : 0;
        first = 0;
        refresh();
        return;
    }
    transport.play(ALBUMS[album].first + selected);
    screenManager.closeModal();
}

bool LibraryScreen::onRightPress()
{
    if (album < 0)
    {
        onConfirmPress();
    }
    return true;
}

bool LibraryScreen::onBackPress()
{
    if (album >= 0)
    {
        selected = (uint8_t)album;
        album = -1;
        first = 0;
        refresh();
        return true;
    }
    screenManager.closeModal();
    return true;
}
