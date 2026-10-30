#include "XpScreen.h"
#include "../nan/Format.h"

static const int16_t ROW_Y = 52;
static const int16_t ROW_H = 44;

XpScreen::XpScreen(Game &game, SoundManager &sound)
    : Screen("Gastar XP"),
      game(game),
      sound(sound),
      wallet(8, 25, 224, 20, false, true),
      balance(label(DISPLAY_W / 2, 31, TC_DATUM, Theme::TEXT, 1, Theme::FIELD)),
      hint(label(DISPLAY_W / 2, 282, TC_DATUM, Theme::TEXT_DIM))
{
    addElement(&wallet);
    addElement(&balance, Theme::FIELD);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].place(*this, ROW_Y + i * ROW_H, ROW_H);
        rows[i].name.setText(XP_UPGRADES[i].name);
    }
    hint.setText("Confirm compra, segure repete");
    addElement(&hint);
    addTaskbar();
}

void XpScreen::onEnter(TFT_eSPI &tft)
{
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void XpScreen::update(unsigned long now)
{
    if (refreshDue(now))
    {
        refresh();
    }
}

void XpScreen::refresh()
{
    char a[12], b[12], buf[64];
    formatNumber(game.xp(), a, sizeof(a));
    snprintf(buf, sizeof(buf), "XP disponivel: %s", a);
    balance.setText(buf);

    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        XpUpgrade u = (XpUpgrade)i;
        ShopRow &row = rows[i];
        row.frame.setOn(i == selected);
        snprintf(buf, sizeof(buf), "x%u", (unsigned)game.xpBought(u));
        row.owned.setText(buf);
        switch (u)
        {
        case XpUpgrade::CAP:
            formatNumber(game.energyCap(), a, sizeof(a));
            formatNumber(game.energyCap() + XP_UPGRADES[i].gain, b, sizeof(b));
            snprintf(buf, sizeof(buf), "Max %s > %s", a, b);
            break;
        case XpUpgrade::REGEN:
            formatNumber(game.energyRegen(), a, sizeof(a));
            formatNumber(game.energyRegen() + XP_UPGRADES[i].gain, b, sizeof(b));
            snprintf(buf, sizeof(buf), "%s/s > %s/s", a, b);
            break;
        case XpUpgrade::POWER:
            snprintf(buf, sizeof(buf), "Treino x%.2f > x%.2f", game.energyPower(),
                     game.energyPower() * (1 + XP_UPGRADES[i].gain /
                                                   (ENERGY_POWER_BASE + XP_UPGRADES[i].gain * game.xpBought(u))));
            break;
        default:
            snprintf(buf, sizeof(buf), "Drops x%.2f > x%.2f", game.luck(), game.luck() + XP_UPGRADES[i].gain);
            break;
        }
        row.detail.setText(buf);
        formatNumber(game.xpCost(u), a, sizeof(a));
        snprintf(buf, sizeof(buf), "%s XP", a);
        row.setPrice(buf, game.xp() >= game.xpCost(u));
    }
}

void XpScreen::onUpPress()
{
    selected = (selected + ROW_COUNT - 1) % ROW_COUNT;
    sound.playMenu();
    refresh();
}

void XpScreen::onDownPress()
{
    selected = (selected + 1) % ROW_COUNT;
    sound.playMenu();
    refresh();
}

bool XpScreen::buy()
{
    bool bought = game.buyXp((XpUpgrade)selected);
    bought ? sound.playBuy() : sound.playDenied();
    refresh();
    return bought;
}

void XpScreen::onConfirmPress()
{
    buy();
}

bool XpScreen::onRepeat(Key key)
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
