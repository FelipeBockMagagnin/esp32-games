#include "CombatScreen.h"
#include "../game/GameConfig.h" // GOLD_COLOR

static const unsigned long REFRESH_MS = 100;

// How long a drop notification keeps the message line before the fight status takes it back
static const unsigned long DROP_MESSAGE_MS = 2500;

static const uint16_t ENEMY_HP_COLOR = 0xF800;  // Red
static const uint16_t PLAYER_HP_COLOR = 0x07E0; // Green
static const uint16_t READY_COLOR = 0x07E0;
static const uint16_t COOLING_COLOR = 0x7BEF; // Gray while on cooldown
static const uint16_t LABEL_COLOR = 0xAD55;
static const uint16_t NEW_ITEM_COLOR = 0x07E0;   // Green for a first-time drop
static const uint16_t LEVEL_UP_COLOR = GOLD_COLOR; // A duplicate levelling the item up

static const int16_t ACTION_STATE_X = 225;

CombatScreen::CombatScreen(CombatState &combat, SoundManager &sound)
    : header("Combat", "Zones", "Mining"),

      enemySprite(56, 32, 128, 128, image_rato_pixels),
      enemyName(120, 164, "", 0xFFFF, 2, TC_DATUM),
      enemyHpBar(55, 182, 130, 15, 0, 1, ENEMY_HP_COLOR),

      enemyAtkIcon(18, 202, 16, 16, image_sword_02b_pixels),
      enemyAtkText(38, 203, "0", 0xFFFF, 2),
      enemyDefIcon(120, 202, 16, 16, image_armor_01b_pixels),
      enemyDefText(140, 203, "0", 0xFFFF, 2),

      timerBox(82, 220, 84, 19, 0xFFFF),
      timerText(112, 226, "0.00s", 0xFFFF, 1),
      timerMultiplier(171, 226, "", LABEL_COLOR, 1),

      messageText(120, 243, "", 0xFFFF, 2, TC_DATUM),

      strikeIcon(11, 263, 16, 16, image_sword_02b_pixels),
      strikeLabel(34, 267, "Strike", LABEL_COLOR, 1),
      strikeState(ACTION_STATE_X, 267, "", READY_COLOR, 1, TR_DATUM),

      guardIcon(11, 281, 16, 16, image_armor_01b_pixels),
      guardLabel(34, 285, "Guard", LABEL_COLOR, 1),
      guardState(ACTION_STATE_X, 285, "", READY_COLOR, 1, TR_DATUM),

      playerHeartIcon(33, 301, 15, 16, image_cards_hearts_bits),
      playerHpBar(55, 301, 130, 16, 0, 1, PLAYER_HP_COLOR),

      combat(combat),
      sound(sound),
      lastRefresh(0),
      seenKills(0),
      seenDrops(0),
      messageUntil(0)
{
    addElement(&header);

    addElement(&enemySprite);
    addElement(&enemyName);
    addElement(&enemyHpBar);
    addElement(&enemyAtkIcon);
    addElement(&enemyAtkText);
    addElement(&enemyDefIcon);
    addElement(&enemyDefText);

    addElement(&timerBox);
    addElement(&timerText);
    addElement(&timerMultiplier);
    addElement(&messageText);

    addElement(&strikeIcon);
    addElement(&strikeLabel);
    addElement(&strikeState);
    addElement(&guardIcon);
    addElement(&guardLabel);
    addElement(&guardState);

    addElement(&playerHeartIcon);
    addElement(&playerHpBar);
}

void CombatScreen::onEnter(TFT_eSPI &tft)
{
    // Kills and drops that happened while the player was elsewhere are already banked;
    // resync here so the screen does not replay their sounds or notifications on arrival
    seenKills = combat.getKillEvents();
    seenDrops = combat.getDropEvents();
    messageUntil = 0;
    lastRefresh = 0;
    Screen::onEnter(tft);
}

void CombatScreen::showFightElements(bool visible)
{
    enemySprite.setVisible(visible);
    enemyName.setVisible(visible);
    enemyHpBar.setVisible(visible);
    enemyAtkIcon.setVisible(visible);
    enemyAtkText.setVisible(visible);
    enemyDefIcon.setVisible(visible);
    enemyDefText.setVisible(visible);
    strikeIcon.setVisible(visible);
    strikeLabel.setVisible(visible);
    strikeState.setVisible(visible);
    guardIcon.setVisible(visible);
    guardLabel.setVisible(visible);
    guardState.setVisible(visible);
    playerHeartIcon.setVisible(visible);
    playerHpBar.setVisible(visible);
}

void CombatScreen::showDropMessage(unsigned long now)
{
    int8_t itemId = combat.getLastDrop();
    if (itemId == NO_ITEM)
    {
        return;
    }

    char buf[32];
    if (combat.wasLastDropNew())
    {
        snprintf(buf, sizeof(buf), "Got %s!", ITEMS[itemId].name);
        messageText.setColor(NEW_ITEM_COLOR);
    }
    else
    {
        // A duplicate is not a new item, so say what it did instead
        snprintf(buf, sizeof(buf), "%s Lv %u", ITEMS[itemId].name, (unsigned)combat.getLastDropLevel());
        messageText.setColor(LEVEL_UP_COLOR);
    }

    messageText.setText(buf);
    messageUntil = now + DROP_MESSAGE_MS;
}

void CombatScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    char buf[24];

    if (!combat.isInZone())
    {
        header.setTitle("Combat");
        showFightElements(false);
        timerBox.setVisible(false);
        timerText.setVisible(false);
        timerMultiplier.setVisible(false);
        messageText.setVisible(true);
        messageText.setColor(0xFFFF);
        messageText.setText("Pick a zone");
        return;
    }

    if (combat.getKillEvents() != seenKills)
    {
        seenKills = combat.getKillEvents();
        sound.playLevelUp();
    }
    if (combat.getDropEvents() != seenDrops)
    {
        seenDrops = combat.getDropEvents();
        sound.playBuy();
        showDropMessage(now);
    }

    const EnemyDef &enemy = combat.getEnemy();
    const Stats &player = combat.getPlayerStats();

    header.setTitle(combat.getZone().name);
    showFightElements(true);

    enemySprite.setPixels(enemy.sprite, enemy.spriteW, enemy.spriteH);
    enemyName.setText(enemy.name);

    enemyHpBar.setProgress((int32_t)combat.getEnemyHp(), (int32_t)enemy.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getEnemyHp(), (unsigned)enemy.maxHp);
    enemyHpBar.setLabel(buf);

    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.attack);
    enemyAtkText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.defense);
    enemyDefText.setText(buf);

    playerHpBar.setProgress((int32_t)combat.getPlayerHp(), (int32_t)player.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getPlayerHp(), (unsigned)player.maxHp);
    playerHpBar.setLabel(buf);

    bool fighting = combat.getPhase() == CombatState::FIGHTING;
    timerBox.setVisible(fighting);
    timerText.setVisible(fighting);
    timerMultiplier.setVisible(fighting);
    messageText.setVisible(true);

    if (fighting)
    {
        unsigned long remaining = combat.getAutoAttackRemaining(now);
        snprintf(buf, sizeof(buf), "%lu.%02lus", remaining / 1000, (remaining % 1000) / 10);
        timerText.setText(buf);

        snprintf(buf, sizeof(buf), "x%u", (unsigned)STRIKE_MULTIPLIER);
        timerMultiplier.setText(buf);
    }

    // A fresh drop notification outranks the fight status for a couple of seconds
    if (messageUntil == 0 || (long)(now - messageUntil) >= 0)
    {
        messageUntil = 0;
        messageText.setColor(0xFFFF);
        if (combat.getPhase() == CombatState::DEAD)
        {
            unsigned long remaining = combat.getReviveRemaining(now);
            snprintf(buf, sizeof(buf), "Revive %lu.%lus", remaining / 1000, (remaining % 1000) / 100);
            messageText.setText(buf);
        }
        else if (fighting)
        {
            messageText.setText("");
        }
        else
        {
            messageText.setText("Next enemy");
        }
    }

    unsigned long strikeLeft = combat.getStrikeCooldownRemaining(now);
    if (!fighting)
    {
        strikeState.setText("-");
        strikeState.setColor(COOLING_COLOR);
    }
    else if (strikeLeft == 0)
    {
        strikeState.setText("ready");
        strikeState.setColor(READY_COLOR);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", strikeLeft / 1000, (strikeLeft % 1000) / 100);
        strikeState.setText(buf);
        strikeState.setColor(COOLING_COLOR);
    }

    unsigned long guardLeft = combat.getGuardCooldownRemaining(now);
    if (combat.isGuarding(now))
    {
        guardState.setText("active");
        guardState.setColor(READY_COLOR);
    }
    else if (!fighting)
    {
        guardState.setText("-");
        guardState.setColor(COOLING_COLOR);
    }
    else if (guardLeft == 0)
    {
        guardState.setText("ready");
        guardState.setColor(READY_COLOR);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", guardLeft / 1000, (guardLeft % 1000) / 100);
        guardState.setText(buf);
        guardState.setColor(COOLING_COLOR);
    }
}

void CombatScreen::onConfirmPress()
{
    if (combat.strike())
    {
        sound.playMine();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}

void CombatScreen::onSelectPress()
{
    if (combat.guard())
    {
        sound.playMenu();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}
