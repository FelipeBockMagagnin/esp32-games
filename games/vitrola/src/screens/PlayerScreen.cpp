#include "PlayerScreen.h"
#include <stdio.h>

PlayerScreen::PlayerScreen(Transport &transport, Turntable &turntable, ScreenManager &screenManager,
                           Screen &library)
    : transport(transport), turntable(turntable), screenManager(screenManager), library(library)
{
}

static void clock(char *out, size_t n, uint32_t frames)
{
    uint32_t s = frames / FRAME_RATE;
    snprintf(out, n, "%lu:%02lu", (unsigned long)(s / 60), (unsigned long)(s % 60));
}

void PlayerScreen::onEnter(TFT_eSPI &tft)
{
    tft.fillScreen(Theme::BG);
    turntable.drawPlinth(tft);
    lcd.invalidate();
    vu.reset();
    drawHints(tft);
    fresh = true;
    render(tft);
}

void PlayerScreen::drawHints(TFT_eSPI &tft)
{
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(Theme::TEXT_DIM);
    tft.drawString("</> faixa  OK toca  ^ modo  v discos", DISPLAY_W / 2, 308);
    tft.setTextDatum(TL_DATUM);
}

void PlayerScreen::update(unsigned long t)
{
    now = t;
    const Playlist &list = transport.playlist;
    const Player &player = transport.player;
    const Track &track = TRACKS[list.current];
    const Album &album = ALBUMS[track.album];

    turntable.setAlbum(track.album);
    turntable.setSpinning(player.isPlaying());
    turntable.setNeedle(transport.needle());

    char credit[64], status[40], at[12], length[12];
    snprintf(credit, sizeof(credit), "%s - %s", track.author, album.name);
    clock(at, sizeof(at), player.isLoaded() ? player.getFrame() : 0);
    clock(length, sizeof(length), track.frames);
    snprintf(status, sizeof(status), "%02u/%02u  %s / %s   %s", list.index() + 1, album.count, at, length,
             modeTag(list.mode));
    lcd.set(track.title, credit, status, player.isPlaying());
}

void PlayerScreen::render(TFT_eSPI &tft)
{
    if (!fresh && now - lastFrame < FRAME_MS)
    {
        return;
    }
    lastFrame = now;
    if (turntable.update(now) || fresh)
    {
        turntable.render(tft);
    }
    lcd.draw(tft, now);
    vu.draw(tft, transport.player);
    fresh = false;
}

void PlayerScreen::onConfirmPress()
{
    transport.toggle();
}

void PlayerScreen::onUpPress()
{
    transport.cycleMode();
}

void PlayerScreen::onDownPress()
{
    screenManager.openModal(&library);
}

bool PlayerScreen::onBackPress()
{
    transport.previous();
    return true;
}

bool PlayerScreen::onRightPress()
{
    transport.next();
    return true;
}
