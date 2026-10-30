#include "TrainingScreen.h"
#include "../ui/Labels.h"

static const int16_t ROW_Y = 92;
static const int16_t ROW_H = 40;

TrainingScreen::TrainingScreen(Game &game, Allocation &allocation, SoundManager &sound)
    : Screen("Treino"),
      game(game),
      allocation(allocation),
      sound(sound),
      battery(8, 25, 226, 28),
      stats(label(8, 58)),
      modeButton(8, 70, 224, 18),
      reclaimButton(8, 258, 224, 20)
{
    addElement(&battery);
    addElement(&stats);
    addElement(&modeButton);
    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        SkillRow &row = rows[i];
        int16_t y = ROW_Y + i * ROW_H;
        row.frame = FocusFrame(5, y, 230, ROW_H - 2);
        row.name = label(10, y + 4);
        row.level = label(230, y + 4, TR_DATUM);
        row.bar = WinBar(10, y + 14, 220, 11, Theme::BAR, Theme::BAR_HI);
        row.info = label(10, y + 28, TL_DATUM, Theme::TEXT_2);
        row.name.setText(SKILLS[i].name);
        addElement(&row.name);
        addElement(&row.level);
        addElement(&row.bar);
        addElement(&row.info);
        addElement(&row.frame);
    }
    reclaimButton.setLabel("Retirar toda a energia");
    addElement(&reclaimButton);
    addTaskbar();
}

void TrainingScreen::onEnter(TFT_eSPI &tft)
{
    forceRefresh();
    refresh();
    Screen::onEnter(tft);
}

void TrainingScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    refresh();
}

void TrainingScreen::refresh()
{
    char a[12], b[12], c[12], buf[64];
    double cap = game.energyCap();
    battery.setLevels(game.allocated() / cap, game.idleEnergy() / cap);
    formatNumber(game.idleEnergy(), a, sizeof(a));
    formatNumber(cap, b, sizeof(b));
    snprintf(buf, sizeof(buf), "Livre %s / %s", a, b);
    battery.setLabel(buf);

    formatNumber(game.attack(), a, sizeof(a));
    formatNumber(game.defense(), b, sizeof(b));
    formatNumber(game.energyRegen(), c, sizeof(c));
    snprintf(buf, sizeof(buf), "ATQ %s  DEF %s  +%s/s", a, b, c);
    stats.setText(buf);

    allocation.describe(game, buf, sizeof(buf));
    modeButton.setLabel(buf);
    modeButton.setFocused(selected == ROW_MODE);
    reclaimButton.setFocused(selected == ROW_RECLAIM);
    reclaimButton.setEnabled(game.allocated() > 0);

    for (uint8_t i = 0; i < SKILL_COUNT; i++)
    {
        SkillRow &row = rows[i];
        row.frame.setOn(selected == i + 1);
        bool open = game.skillOpen(i);
        row.name.setColor(open ? Theme::TEXT : Theme::TEXT_DIM);
        if (!open)
        {
            const SkillDef &def = SKILLS[i];
            snprintf(buf, sizeof(buf), "Abre com %s Nv %lu", SKILLS[def.parent].name, (unsigned long)def.parentLevel);
            row.info.setText(buf);
            row.level.setText("Bloqueado");
            row.bar.setValue(0);
            continue;
        }
        snprintf(buf, sizeof(buf), "Nv %lu", (unsigned long)game.skillLevel(i));
        row.level.setText(buf);
        double seconds = game.skillSecondsPerLevel(i);
        // A level faster than the refresh would only flicker; show it full
        row.bar.setValue(seconds > 0 && seconds < 0.3 ? 1 : game.skillProgress(i));

        formatNumber(game.allocatedTo({false, i}), a, sizeof(a));
        formatNumber(game.skillStat(i), b, sizeof(b));
        const char *stat = SKILLS[i].kind == SkillKind::ATTACK ? "ATQ" : "DEF";
        if (seconds <= 0)
        {
            snprintf(buf, sizeof(buf), "E %s  +%s %s", a, b, stat);
        }
        else if (seconds < 1)
        {
            snprintf(buf, sizeof(buf), "E %s  %.0f nv/s  +%s %s", a, 1 / seconds, b, stat);
        }
        else
        {
            snprintf(buf, sizeof(buf), "E %s  %.1fs/nv  +%s %s", a, seconds, b, stat);
        }
        row.info.setText(buf);
    }
}

void TrainingScreen::select(int8_t delta)
{
    selected = (selected + ROW_COUNT + delta) % ROW_COUNT;
    sound.playMenu();
    forceRefresh();
    refresh();
}

void TrainingScreen::onUpPress()
{
    select(-1);
}

void TrainingScreen::onDownPress()
{
    select(1);
}

bool TrainingScreen::act()
{
    if (selected == ROW_MODE)
    {
        allocation.next();
        sound.playMenu();
        return false;
    }
    if (selected == ROW_RECLAIM)
    {
        bool any = game.allocated() > 0;
        game.reclaimAll();
        any ? sound.playMenu() : sound.playDenied();
        return false;
    }
    bool moved = allocation.apply(game, {false, (uint8_t)(selected - 1)}) > 0;
    moved ? sound.playMenu() : sound.playDenied();
    return moved;
}

void TrainingScreen::onConfirmPress()
{
    act();
    forceRefresh();
    refresh();
}

bool TrainingScreen::onRepeat(Key key)
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
    forceRefresh();
    refresh();
    return keep;
}
