#include "PlayScreen.h"
#include "../render/Art.h"

static const int16_t HUD_Y = View::H;
static const int16_t ROW_1 = HUD_Y + 5;
static const int16_t ROW_2 = HUD_Y + 21;
static const int16_t NOTICE_Y = HUD_Y + 38;
static const int16_t BAR_W = 116;
static const int16_t BAR_H = 12;
static const int16_t POTION_X = 128;
static const int16_t FLOOR_X = 172;
static const int16_t GEAR_X = 244;

static const uint16_t *itemIcon(const Item &item)
{
    return ITEM_ICONS[ITEM_SERIES_FIRST[item.slot] + item.series * ITEM_TIERS + item.tier];
}

PlayScreen::PlayScreen(Run &run, Frame &frame, SoundManager &sound, const Hooks &hooks)
    : run(run),
      frame(frame),
      sound(sound),
      hooks(hooks),
      rule(0, HUD_Y, DISPLAY_W, 1, Theme::FRAME_DIM, true, Theme::FRAME_DIM),
      hpBar(4, ROW_1, BAR_W, BAR_H, 0, 1, Theme::HP_BAR, Theme::HP_TRACK, Theme::HP_BORDER),
      manaBar(4, ROW_2, BAR_W, BAR_H, 0, 1, Theme::MANA_BAR),
      hpPotionIcon(POTION_X, ROW_1 - 2, 16, 16, ITEM_ICONS[ITEM_POTION_FIRST], true),
      mpPotionIcon(POTION_X, ROW_2 - 2, 16, 16, ITEM_ICONS[ITEM_POTION_FIRST + 1], true),
      hpPotionText(POTION_X + 18, ROW_1 + 2, "", Theme::TEXT),
      mpPotionText(POTION_X + 18, ROW_2 + 2, "", Theme::TEXT),
      floorText(FLOOR_X, ROW_1 + 2, "", Theme::GOLD),
      killsText(FLOOR_X, ROW_2 + 2, "", Theme::TEXT_2),
      weaponIcon(GEAR_X, ROW_1 - 2, 16, 16, ITEM_ICONS[0], true),
      focusIcon(GEAR_X, ROW_2 - 2, 16, 16, ITEM_ICONS[ITEM_SERIES_FIRST[SLOT_FOCUS]], true),
      weaponText(GEAR_X + 20, ROW_1 + 2, "", Theme::TEXT),
      focusText(GEAR_X + 20, ROW_2 + 2, "", Theme::TEXT),
      noticeText(DISPLAY_W / 2, NOTICE_Y, "", Theme::TEXT, 1, TC_DATUM)
{
    addElement(&rule);
    addElement(&hpBar);
    addElement(&manaBar);
    addElement(&hpPotionIcon);
    addElement(&mpPotionIcon);
    addElement(&hpPotionText);
    addElement(&mpPotionText);
    addElement(&floorText);
    addElement(&killsText);
    addElement(&weaponIcon);
    addElement(&focusIcon);
    addElement(&weaponText);
    addElement(&focusText);
    addElement(&noticeText);
}

void PlayScreen::begin(bool teach)
{
    diedAt = 0;
    noticePending = true;
    tutorial = Tutorial();
    if (teach && run.getFloor() == 1)
    {
        tutorial.start(run);
    }
}

void PlayScreen::setHeld(bool up, bool down, bool left, bool right, bool confirm)
{
    if (!armed)
    {
        armed = !up && !down && !confirm;
        held = {};
        return;
    }
    held.forward = up;
    held.back = down;
    held.left = left;
    held.right = right;
    held.attack = confirm;
}

void PlayScreen::onEnter(TFT_eSPI &tft)
{
    armed = false;
    held = {};
    lastUpdate = 0;
    run.takeEvents();
    Screen::onEnter(tft);
}

void PlayScreen::notice(const char *text, Color color, unsigned long now)
{
    noticeText.setText(text);
    noticeText.setColor(color);
    noticeUntil = now + NOTICE_MS;
}

void PlayScreen::noticeItem(const char *verb, const Item &item, unsigned long now)
{
    char name[24], stat[16], line[56];
    itemName(item, name, sizeof(name));
    itemStat(item, stat, sizeof(stat));
    snprintf(line, sizeof(line), "%s %s %s", verb, name, stat);
    notice(line, Theme::tierColor(item.tier), now);
}

void PlayScreen::handleEvents(uint32_t events, unsigned long now)
{
    if (events & EV_DIED)
    {
        sound.playDie();
        diedAt = now;
        notice("You have fallen", Theme::BAD, now);
        return;
    }
    if (events & EV_EXIT)
    {
        sound.playDescend();
        run.nextFloor();
        hooks.descended();
        noticePending = true;
        return;
    }
    if (events & EV_BOSS_DOWN)
    {
        sound.playBossDown();
        notice("The Lich falls. The portal opens.", Theme::GOLD, now);
    }
    else if (events & EV_EQUIP)
    {
        sound.playEquip();
        noticeItem("Equipped", run.getLastPicked(), now);
    }
    else if (events & EV_BAG)
    {
        sound.playBag();
        noticeItem("Bagged", run.getLastPicked(), now);
    }
    else if (events & EV_BAG_FULL)
    {
        sound.playError();
        notice(run.getLastPotion() != 0 ? "Potion belt full" : "Bag full - drop something", Theme::BAD, now);
    }
    else if (events & EV_POTION)
    {
        sound.playPotion();
        bool health = run.getLastPotion() == 1;
        notice(health ? "Health potion" : "Mana potion", health ? Theme::HP_BAR : Theme::MANA_BAR, now);
    }
    else if (events & EV_SEALED)
    {
        sound.playSealed();
        notice("Sealed. Slay the Lich.", Theme::BAD, now);
    }
    else if (events & EV_NO_MANA)
    {
        sound.playError();
        notice("Not enough mana", Theme::MANA_BAR, now);
    }
    else if (events & EV_HURT)
    {
        sound.playHurt();
    }
    else if (events & EV_BLAST)
    {
        sound.playBlast();
    }
    else if (events & EV_KILL)
    {
        sound.playKill();
    }
    else if (events & EV_HIT)
    {
        sound.playHit();
    }
    else if (events & EV_CAST)
    {
        sound.playCast();
    }
    else if (events & EV_SWING)
    {
        sound.playSwing();
    }
    else if (events & EV_CHARGED)
    {
        sound.playCharged();
    }
    else if (events & EV_ENEMY_SHOT)
    {
        sound.playEnemyShot();
    }
    else if (events & EV_ALERT)
    {
        sound.playAlert();
    }
}

void PlayScreen::refreshHud(unsigned long now)
{
    char text[24];
    snprintf(text, sizeof(text), "HP %d/%u", run.getHp(), run.maxHp());
    hpBar.setProgress(run.getHp(), run.maxHp());
    hpBar.setLabel(text);
    snprintf(text, sizeof(text), "MP %u/%u", run.getMana(), run.maxMana());
    manaBar.setProgress(run.getMana(), run.maxMana());
    manaBar.setLabel(text);
    snprintf(text, sizeof(text), "%u", run.inventory.hpPotions);
    hpPotionText.setText(text);
    snprintf(text, sizeof(text), "%u", run.inventory.mpPotions);
    mpPotionText.setText(text);

    snprintf(text, sizeof(text), "Floor %u", run.getFloor());
    floorText.setText(text);
    snprintf(text, sizeof(text), "Kills %u", run.getKills());
    killsText.setText(text);

    const Item &weapon = run.inventory.equipped[SLOT_WEAPON];
    const Item &focus = run.inventory.equipped[SLOT_FOCUS];
    weaponIcon.setPixels(weapon.empty() ? ITEM_ICONS[0] : itemIcon(weapon), 16, 16, true);
    focusIcon.setPixels(focus.empty() ? ITEM_ICONS[ITEM_SERIES_FIRST[SLOT_FOCUS]] : itemIcon(focus), 16, 16, true);
    snprintf(text, sizeof(text), "%u", run.swordDamage());
    weaponText.setText(text);
    snprintf(text, sizeof(text), "%u", run.spellDamage());
    focusText.setText(text);

    if (noticePending)
    {
        noticePending = false;
        if (run.isBossFloor() && run.isSealed())
        {
            snprintf(text, sizeof(text), "Floor %u - the Lich", run.getFloor());
            notice(text, Theme::BAD, now);
        }
        else
        {
            snprintf(text, sizeof(text), "Floor %u", run.getFloor());
            notice(text, Theme::GOLD, now);
        }
    }
    if (noticeUntil != 0 && (long)(now - noticeUntil) >= 0)
    {
        noticeUntil = 0;
        noticeText.setText("");
    }
    // Any other notice goes first; the hint comes back once it is gone
    if (noticeUntil == 0 && !tutorial.isDone())
    {
        noticeText.setText(tutorial.hint());
        noticeText.setColor(Theme::GOLD);
    }
}

void PlayScreen::update(unsigned long now)
{
    if (diedAt != 0)
    {
        if (now - diedAt >= DEATH_PAUSE_MS)
        {
            diedAt = 0;
            hooks.died();
        }
        return;
    }
    if (armed && held.forward && held.back)
    {
        hooks.paused();
        return;
    }
    uint16_t dt = lastUpdate == 0 ? 0 : (uint16_t)(now - lastUpdate > MAX_STEP_MS ? MAX_STEP_MS : now - lastUpdate);
    lastUpdate = now;
    if (dt > 0)
    {
        run.update(held, dt);
    }
    uint32_t events = run.takeEvents();
    if (!tutorial.isDone() && tutorial.update(run, events, dt))
    {
        if (tutorial.isDone())
        {
            noticeText.setText("");
            hooks.taught();
        }
        else
        {
            sound.playMenu();
        }
    }
    handleEvents(events, now);
    view.prepare(run);
    frame.draw(view);
    refreshHud(now);
}
