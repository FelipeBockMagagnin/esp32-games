#include "CombatScreen.h"
#include "../game/GameConfig.h" // GOLD_COLOR

static const unsigned long REFRESH_MS = 100;

// How long a drop notification keeps the message line before the fight status takes it back
static const unsigned long DROP_MESSAGE_MS = 2500;

// Warm belongs to the enemy, cool to the player
static const uint16_t ENEMY_HP_COLOR = 0xF800;      // Red
static const uint16_t ENEMY_ATTACK_COLOR = 0xFFE0;  // Yellow
static const uint16_t PLAYER_ATTACK_COLOR = 0x07FF; // Cyan
static const uint16_t PLAYER_HP_COLOR = 0x07E0;     // Green
// Dark gray so the white bar label stays readable over a full, static bar
static const uint16_t INERT_COLOR = 0x39E7;

static const uint16_t READY_COLOR = 0x07E0;
static const uint16_t COOLING_COLOR = 0x7BEF; // Gray while on cooldown
static const uint16_t LABEL_COLOR = 0xAD55;
static const uint16_t NEW_ITEM_COLOR = 0x07E0;     // Green for a first-time drop
static const uint16_t LEVEL_UP_COLOR = GOLD_COLOR; // A duplicate levelling the item up
static const uint16_t BLOCKED_COLOR = 0xF800;      // No attack can get through at all

// The bars share one width so they stack as a single column
static const int16_t BAR_X = 15;
static const int16_t BAR_W = 210;
static const int16_t HP_BAR_H = 15;
static const int16_t TIMER_BAR_H = 12;

static const int16_t ACTION_LABEL_X = 15;
static const int16_t ACTION_STATE_X = 225;

static const int16_t CENTER_X = 120;

CombatScreen::CombatScreen(CombatState &combat, SoundManager &sound)
    : header("Combat", "Zones", "Mining"),

      // Enemy block
      enemyName(CENTER_X, 32, "", 0xFFFF, 2, TC_DATUM),
      enemySprite(56, 52, 128, 128, image_rato_pixels),
      enemyHpBar(BAR_X, 184, BAR_W, HP_BAR_H, 0, 1, ENEMY_HP_COLOR),
      enemyAttackBar(BAR_X, 202, BAR_W, TIMER_BAR_H, 0, 1, ENEMY_ATTACK_COLOR),

      messageText(CENTER_X, 218, "", 0xFFFF, 2, TC_DATUM),

      // Actions, between the two blocks: one line each, in button order
      strikeLabel(ACTION_LABEL_X, 239, "", LABEL_COLOR, 1),
      strikeState(ACTION_STATE_X, 239, "", READY_COLOR, 1, TR_DATUM),
      guardLabel(ACTION_LABEL_X, 250, "Guard", LABEL_COLOR, 1),
      guardState(ACTION_STATE_X, 250, "", READY_COLOR, 1, TR_DATUM),
      smiteLabel(ACTION_LABEL_X, 261, "", LABEL_COLOR, 1),
      smiteState(ACTION_STATE_X, 261, "", READY_COLOR, 1, TR_DATUM),

      // Player block, mirroring the enemy one
      playerAttackBar(BAR_X, 274, BAR_W, TIMER_BAR_H, 0, 1, PLAYER_ATTACK_COLOR),
      playerHpBar(BAR_X, 289, BAR_W, HP_BAR_H, 0, 1, PLAYER_HP_COLOR),
      statsLine(CENTER_X, 308, "", LABEL_COLOR, 1, TC_DATUM),

      combat(combat),
      sound(sound),
      lastRefresh(0),
      seenKills(0),
      seenDrops(0),
      messageUntil(0)
{
    addElement(&header);
    setHeader(&header);

    addElement(&enemyName);
    addElement(&enemySprite);
    addElement(&enemyHpBar);
    addElement(&enemyAttackBar);

    addElement(&messageText);

    addElement(&strikeLabel);
    addElement(&strikeState);
    addElement(&guardLabel);
    addElement(&guardState);
    addElement(&smiteLabel);
    addElement(&smiteState);

    addElement(&playerAttackBar);
    addElement(&playerHpBar);
    addElement(&statsLine);

    // Each multiplier belongs to its action, not to a countdown
    char buf[16];
    snprintf(buf, sizeof(buf), "Strike x%u", (unsigned)STRIKE_MULTIPLIER);
    strikeLabel.setText(buf);
    snprintf(buf, sizeof(buf), "Smite x%u", (unsigned)SMITE_MULTIPLIER);
    smiteLabel.setText(buf);
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
    enemyName.setVisible(visible);
    enemySprite.setVisible(visible);
    enemyHpBar.setVisible(visible);
    strikeLabel.setVisible(visible);
    strikeState.setVisible(visible);
    guardLabel.setVisible(visible);
    guardState.setVisible(visible);
    smiteLabel.setVisible(visible);
    smiteState.setVisible(visible);
    playerHpBar.setVisible(visible);
    statsLine.setVisible(visible);
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

void CombatScreen::refreshTimerBars(unsigned long now, bool fighting)
{
    char buf[40];

    // Each bar says who hits whom, for how much, and how long until it lands
    enemyAttackBar.setVisible(fighting);
    if (fighting)
    {
        uint32_t enemyDamage = combat.getEnemyAttackDamage();
        if (enemyDamage == 0)
        {
            // A full static bar rather than an empty one: counting down to nothing read
            // as a bug, and an empty bar reads as a countdown about to fire
            enemyAttackBar.setProgress(1, 1);
            enemyAttackBar.setFillColor(INERT_COLOR);
            enemyAttackBar.setLabel("enemy cannot hurt you");
        }
        else
        {
            unsigned long left = combat.getEnemyAttackRemaining(now);
            enemyAttackBar.setProgress((int32_t)left, (int32_t)combat.getEnemy().attackIntervalMs);
            enemyAttackBar.setFillColor(ENEMY_ATTACK_COLOR);
            snprintf(buf, sizeof(buf), "enemy hits %u in %lu.%lus",
                     (unsigned)enemyDamage, left / 1000, (left % 1000) / 100);
            enemyAttackBar.setLabel(buf);
        }
    }

    // The automatic attack gets no bar at all when it cannot damage the enemy: there is
    // no countdown worth showing, and the action rows are what matter then
    uint32_t autoDamage = combat.getAutoAttackDamage();
    bool showPlayerBar = fighting && autoDamage > 0;
    playerAttackBar.setVisible(showPlayerBar);
    if (showPlayerBar)
    {
        unsigned long left = combat.getAutoAttackRemaining(now);
        playerAttackBar.setProgress((int32_t)left, (int32_t)AUTO_ATTACK_MS);
        snprintf(buf, sizeof(buf), "you hit %u in %lu.%lus",
                 (unsigned)autoDamage, left / 1000, (left % 1000) / 100);
        playerAttackBar.setLabel(buf);
    }
}

void CombatScreen::refreshActionStates(unsigned long now, bool fighting)
{
    char buf[16];

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

    // Shown even while locked, so the amulet reward is discoverable
    if (!combat.isSmiteUnlocked())
    {
        smiteLabel.setColor(COOLING_COLOR);
        smiteState.setText("needs amulet");
        smiteState.setColor(COOLING_COLOR);
        return;
    }

    smiteLabel.setColor(LABEL_COLOR);
    unsigned long smiteLeft = combat.getSmiteCooldownRemaining(now);
    if (!fighting)
    {
        smiteState.setText("-");
        smiteState.setColor(COOLING_COLOR);
    }
    else if (smiteLeft == 0)
    {
        smiteState.setText("ready");
        smiteState.setColor(READY_COLOR);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", smiteLeft / 1000, (smiteLeft % 1000) / 100);
        smiteState.setText(buf);
        smiteState.setColor(COOLING_COLOR);
    }
}

void CombatScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    char buf[40];

    if (!combat.isInZone())
    {
        header.setTitle("Combat");
        showFightElements(false);
        enemyAttackBar.setVisible(false);
        playerAttackBar.setVisible(false);
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
    bool fighting = combat.getPhase() == CombatState::FIGHTING;

    header.setTitle(combat.getZone().name);
    showFightElements(true);
    messageText.setVisible(true);

    enemyName.setText(enemy.name);
    enemySprite.setPixels(enemy.sprite, enemy.spriteW, enemy.spriteH);

    enemyHpBar.setProgress((int32_t)combat.getEnemyHp(), (int32_t)enemy.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getEnemyHp(), (unsigned)enemy.maxHp);
    enemyHpBar.setLabel(buf);

    playerHpBar.setProgress((int32_t)combat.getPlayerHp(), (int32_t)player.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getPlayerHp(), (unsigned)player.maxHp);
    playerHpBar.setLabel(buf);

    snprintf(buf, sizeof(buf), "ATK %u  DEF %u  ENEMY DEF %u",
             (unsigned)player.attack, (unsigned)player.defense, (unsigned)enemy.defense);
    statsLine.setText(buf);

    refreshTimerBars(now, fighting);

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
            // The attack bars cover a blocked automatic hit; this is the harder case
            // where no action gets through, so the zone itself is out of reach
            if (combat.getBestAttackDamage() == 0)
            {
                messageText.setColor(BLOCKED_COLOR);
                messageText.setText("Attack too low");
            }
            else
            {
                messageText.setText("");
            }
        }
        else
        {
            messageText.setText("Next enemy");
        }
    }

    refreshActionStates(now, fighting);
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

bool CombatScreen::onBackPress()
{
    // Without an amulet there is no smite, so the button keeps its usual job of
    // stepping back a screen
    if (!combat.isSmiteUnlocked())
    {
        return false;
    }

    if (combat.smite())
    {
        sound.playLevelUp();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
    return true;
}
