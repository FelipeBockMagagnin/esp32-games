#include "MinerScreen.h"
#include "../nan/Format.h"

static const int16_t ROW_Y = 52;
static const int16_t ROW_H = 44;

MinerScreen::MinerScreen(Game &game, SoundManager &sound)
    : Screen("Mineradores"),
      game(game),
      sound(sound),
      wallet(8, 25, 224, 20, false, true),
      balance(label(DISPLAY_W / 2, 31, TC_DATUM, Theme::GOLD, 1, Theme::FIELD)),
      hint(label(DISPLAY_W / 2, 282, TC_DATUM, Theme::TEXT_DIM))
{
    addElement(&wallet);
    addElement(&balance, Theme::FIELD);
    for (uint8_t i = 0; i < MINER_COUNT; i++)
    {
        rows[i].place(*this, ROW_Y + i * ROW_H, ROW_H);
        rows[i].name.setText(MINERS[i].name);
    }
    hint.setText("Ouro paga aprimoramentos");
    addElement(&hint);
    addTaskbar();
}

void MinerScreen::onEnter(TFT_eSPI &tft)
{
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void MinerScreen::update(unsigned long now)
{
    if (refreshDue(now))
    {
        refresh();
    }
}

void MinerScreen::refresh()
{
    char a[12], b[12], buf[64];
    formatNumber(game.gold(), a, sizeof(a));
    formatNumber(game.goldPerSecond(), b, sizeof(b));
    snprintf(buf, sizeof(buf), "Ouro %s  (+%s/s)", a, b);
    balance.setText(buf);

    for (uint8_t i = 0; i < MINER_COUNT; i++)
    {
        ShopRow &row = rows[i];
        row.frame.setOn(i == selected);
        snprintf(buf, sizeof(buf), "x%u", (unsigned)game.minerCount(i));
        row.owned.setText(buf);
        formatNumber(MINERS[i].goldPerSecond, a, sizeof(a));
        snprintf(buf, sizeof(buf), "+%s ouro/s cada", a);
        row.detail.setText(buf);
        formatNumber(game.minerCost(i), a, sizeof(a));
        row.setPrice(a, game.gold() >= game.minerCost(i));
    }
}

void MinerScreen::onUpPress()
{
    selected = (selected + MINER_COUNT - 1) % MINER_COUNT;
    sound.playMenu();
    refresh();
}

void MinerScreen::onDownPress()
{
    selected = (selected + 1) % MINER_COUNT;
    sound.playMenu();
    refresh();
}

bool MinerScreen::buy()
{
    bool bought = game.buyMiner(selected);
    bought ? sound.playBuy() : sound.playDenied();
    refresh();
    return bought;
}

void MinerScreen::onConfirmPress()
{
    buy();
}

bool MinerScreen::onRepeat(Key key)
{
    if (key == Key::CONFIRM)
    {
        return buy();
    }
    if (key == Key::UP || key == Key::DOWN)
    {
        key == Key::UP ? onUpPress() : onDownPress();
        return true;
    }
    return false;
}
