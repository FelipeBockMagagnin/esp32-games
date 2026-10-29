#include "DungeonScreen.h"

static const int16_t HP_X = 4;
static const int16_t HP_Y = 3;
static const int16_t HP_W = 84;
static const int16_t HP_H = 11;
static const int16_t STATS_RIGHT = 236;
static const int16_t STATS_Y = 5;
static const int16_t LOG_X = 4;
static const int16_t LOG_Y = 16;
static const int16_t LOG_PITCH = 8;

DungeonScreen::DungeonScreen(Run &run, SoundManager &sound, ScreenManager &screenManager, Screen &menu,
                             void (*floorReached)(), void (*died)())
    : statsText(STATS_RIGHT, STATS_Y, "", Theme::TEXT, 1, TR_DATUM),
      hpBar(HP_X, HP_Y, HP_W, HP_H, 0, 1, Theme::HP_BAR, Theme::HP_TRACK, Theme::HP_BORDER),
      tileMap(0, HUD_H, run),
      run(run),
      sound(sound),
      screenManager(screenManager),
      menu(menu),
      floorReached(floorReached),
      died(died),
      seenRevision(0),
      lastEvents(EV_NONE)
{
    addElement(&statsText);
    hpBar.enableTrail(Theme::HIT_TRAIL);
    addElement(&hpBar);
    for (uint8_t i = 0; i < Run::MESSAGE_LINES; i++)
    {
        logLines[i].setPosition(LOG_X, LOG_Y + i * LOG_PITCH);
        logLines[i].setColor(i == 0 ? Theme::TEXT : Theme::TEXT_2);
        addElement(&logLines[i]);
    }
    addElement(&tileMap);
}

void DungeonScreen::onEnter(TFT_eSPI &tft)
{
    lastEvents = EV_NONE;
    refresh();
    Screen::onEnter(tft);
}

void DungeonScreen::refresh()
{
    seenRevision = run.getRevision();
    char buf[24];
    snprintf(buf, sizeof(buf), "%d/%d", run.getHp(), run.getMaxHp());
    hpBar.setProgress(run.getHp(), run.getMaxHp());
    hpBar.setLabel(buf);
    snprintf(buf, sizeof(buf), "F%u Lv%u A%d D%d P%u", (unsigned)run.getFloor(), (unsigned)run.getLevel(),
             run.getAttack(), run.getDefense(), (unsigned)run.getPotions());
    statsText.setText(buf);
    for (uint8_t i = 0; i < Run::MESSAGE_LINES; i++)
    {
        logLines[i].setText(run.getMessage(i));
    }
    tileMap.markDirty();
}

void DungeonScreen::update(unsigned long now)
{
    hpBar.update(now);
    if (run.getRevision() != seenRevision)
    {
        refresh();
    }
}

void DungeonScreen::act(Action action)
{
    uint16_t events = run.act(action);
    lastEvents = events;

    if (events & EV_DIED)
    {
        sound.playDeath();
        refresh();
        died();
        return;
    }
    if (events & EV_FLOOR)
    {
        sound.playStairs();
        floorReached();
    }
    else if (events & EV_LEVEL_UP)
    {
        sound.playLevelUp();
    }
    else if (events & EV_GEAR)
    {
        sound.playGear();
    }
    else if (events & EV_KILL)
    {
        sound.playKill();
    }
    else if (events & EV_HURT)
    {
        sound.playHurt();
    }
    else if (events & EV_HIT)
    {
        sound.playHit();
    }
    else if (events & EV_HEAL)
    {
        sound.playHeal();
    }
    else if (events & EV_PICKUP)
    {
        sound.playPickup();
    }
    // A wall bump stays silent, since a held direction runs into one at every corridor's end
    else if ((events & EV_BLOCKED) && action > Action::EAST)
    {
        sound.playError();
    }
}

void DungeonScreen::onConfirmPress()
{
    sound.playMenu();
    screenManager.openModal(&menu);
}

// Walking on stops at anything but a plain step, so holding a direction never fights,
// takes a hit or walks past an item without the player letting go and choosing to
bool DungeonScreen::onRepeat(Key key)
{
    if (lastEvents != EV_MOVED || run.onStairs())
    {
        return false;
    }
    switch (key)
    {
    case Key::UP:
        act(Action::NORTH);
        break;
    case Key::DOWN:
        act(Action::SOUTH);
        break;
    case Key::LEFT:
        act(Action::WEST);
        break;
    case Key::RIGHT:
        act(Action::EAST);
        break;
    default:
        return false;
    }
    return lastEvents == EV_MOVED;
}
