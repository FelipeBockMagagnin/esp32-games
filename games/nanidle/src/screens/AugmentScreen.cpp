#include "AugmentScreen.h"
#include "../ui/Labels.h"

static const int16_t ROW_Y = 64;
static const int16_t ROW_H = 58;
static const char *const EFFECT_NAMES[] = {"ATQ", "DEF", "poder"};

AugmentScreen::AugmentScreen(Game &game, Allocation &allocation, SoundManager &sound)
    : Screen("Aprimoramentos"),
      game(game),
      allocation(allocation),
      sound(sound),
      energy(label(8, 25)),
      gold(label(8, 36, TL_DATUM, Theme::GOLD)),
      modeButton(8, 46, 224, 16),
      hint(label(DISPLAY_W / 2, 270, TC_DATUM, Theme::TEXT_DIM))
{
    addElement(&energy);
    addElement(&gold);
    addElement(&modeButton);
    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        AugRow &row = rows[i];
        int16_t y = ROW_Y + 2 + i * ROW_H;
        row.frame = FocusFrame(5, y, 230, ROW_H - 4);
        row.name = label(10, y + 4);
        row.level = label(230, y + 4, TR_DATUM);
        row.bar = WinBar(10, y + 15, 220, 11, Theme::GOLD, Theme::BAR_HI);
        row.effect = label(10, y + 29, TL_DATUM, Theme::TEXT_2);
        row.info = label(10, y + 40, TL_DATUM, Theme::TEXT_2);
        row.name.setText(AUGMENTS[i].name);
        addElement(&row.name);
        addElement(&row.level);
        addElement(&row.bar);
        addElement(&row.effect);
        addElement(&row.info);
        addElement(&row.frame);
    }
    hint.setText("Cada nivel cobra ouro ao encher");
    addElement(&hint);
    addTaskbar();
}

void AugmentScreen::onEnter(TFT_eSPI &tft)
{
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void AugmentScreen::update(unsigned long now)
{
    if (refreshDue(now))
    {
        refresh();
    }
}

void AugmentScreen::refresh()
{
    char a[12], b[12], buf[64];
    formatNumber(game.idleEnergy(), a, sizeof(a));
    formatNumber(game.energyCap(), b, sizeof(b));
    snprintf(buf, sizeof(buf), "Energia livre %s / %s", a, b);
    energy.setText(buf);
    formatNumber(game.gold(), a, sizeof(a));
    snprintf(buf, sizeof(buf), "Ouro %s", a);
    gold.setText(buf);
    allocation.describe(game, buf, sizeof(buf));
    modeButton.setLabel(buf);
    modeButton.setFocused(selected == ROW_MODE);

    for (uint8_t i = 0; i < AUGMENT_COUNT; i++)
    {
        AugRow &row = rows[i];
        row.frame.setOn(selected == i + 1);
        snprintf(buf, sizeof(buf), "Nv %u", (unsigned)game.augmentLevel(i));
        row.level.setText(buf);
        row.bar.setValue(game.augmentProgress(i));
        formatNumber(AUGMENTS[i].percentPerLevel * game.augmentLevel(i), a, sizeof(a));
        formatNumber(game.augmentGoldCost(i), b, sizeof(b));
        snprintf(buf, sizeof(buf), "+%s%% %s   prox: %s ouro", a, EFFECT_NAMES[(uint8_t)AUGMENTS[i].effect], b);
        row.effect.setText(buf);
        formatNumber(game.allocatedTo({true, i}), a, sizeof(a));
        if (game.augmentStalled(i))
        {
            snprintf(buf, sizeof(buf), "E %s  esperando ouro", a);
            row.info.setColor(Theme::BAD);
        }
        else
        {
            snprintf(buf, sizeof(buf), "E %s", a);
            row.info.setColor(Theme::TEXT_2);
        }
        row.info.setText(buf);
    }
}

void AugmentScreen::select(int8_t delta)
{
    selected = (selected + ROW_COUNT + delta) % ROW_COUNT;
    sound.playMenu();
    refresh();
}

void AugmentScreen::onUpPress()
{
    select(-1);
}

void AugmentScreen::onDownPress()
{
    select(1);
}

bool AugmentScreen::act()
{
    if (selected == ROW_MODE)
    {
        allocation.next();
        sound.playMenu();
        return false;
    }
    bool moved = allocation.apply(game, {true, (uint8_t)(selected - 1)}) > 0;
    moved ? sound.playMenu() : sound.playDenied();
    return moved;
}

void AugmentScreen::onConfirmPress()
{
    act();
    refresh();
}

bool AugmentScreen::onRepeat(Key key)
{
    if (key == Key::UP || key == Key::DOWN)
    {
        select(key == Key::UP ? -1 : 1);
        return true;
    }
    if (key != Key::CONFIRM)
    {
        return false;
    }
    bool keep = act();
    refresh();
    return keep;
}
