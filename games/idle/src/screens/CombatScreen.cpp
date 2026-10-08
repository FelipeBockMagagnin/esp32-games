#include "CombatScreen.h"
#include "combat/CombatStyle.h"
#include "../game/Format.h"

// A new item holds the stage longer than a duplicate that only levelled one up
static const unsigned long NEW_DROP_MS = 3000;
static const unsigned long LEVEL_DROP_MS = 1800;
static const unsigned long KILL_MESSAGE_MS = 1500;

// The player's bar flashes for the first part of a hit's text, so a hit taken is noticed
// even when the eyes are elsewhere
static const unsigned long HIT_TEXT_MS = 800;
static const unsigned long HURT_FLASH_MS = 300;
// Enough warning to see the enemy's hit coming and press Down in time
static const unsigned long WARN_MS = 600;

// Warm belongs to the enemy, cool to the player
static const uint16_t ENEMY_HP_COLOR = Theme::ENEMY_HP_BAR;
static const uint16_t ENEMY_HP_BG = Theme::ENEMY_HP_TRACK;
static const uint16_t ENEMY_HP_BORDER = Theme::ENEMY_HP_BORDER;
static const uint16_t ENEMY_ATTACK_COLOR = Theme::ENEMY_ATK_BAR;
static const uint16_t ENEMY_ATTACK_BG = Theme::ENEMY_ATK_TRACK;
static const uint16_t ENEMY_ATTACK_BORDER = Theme::ENEMY_ATK_BORDER;
static const uint16_t PLAYER_HP_COLOR = Theme::HP_BAR;
static const uint16_t PLAYER_HP_BG = Theme::HP_TRACK;
static const uint16_t PLAYER_HP_BORDER = Theme::HP_BORDER;
static const uint16_t INERT_COLOR = Theme::INERT_BAR;

static const int16_t CENTER_X = 120;

// Bars leave room on their right for the damage of the last hit
static const int16_t BAR_X = 6;
static const int16_t HP_BAR_W = 196;
static const int16_t HIT_TEXT_X = 234;
static const int16_t ENEMY_HP_Y = 182;
static const int16_t ENEMY_HP_H = 14;
static const int16_t ENEMY_ATK_BAR_Y = 199;
static const int16_t ENEMY_ATK_BAR_W = 228;
static const int16_t ENEMY_ATK_BAR_H = 13;

static const int16_t MESSAGE_Y = 215;

static const int16_t STATS_Y = 298;
static const int16_t PLAYER_HP_Y = 308;
static const int16_t PLAYER_HP_H = 11;

CombatScreen::CombatScreen(CombatState &combat, SoundManager &sound)
    : header("Combat"),
      enemyPanel(combat),
      enemyHpBar(BAR_X, ENEMY_HP_Y, HP_BAR_W, ENEMY_HP_H, 0, 1, ENEMY_HP_COLOR, ENEMY_HP_BG, ENEMY_HP_BORDER),
      enemyHitText(HIT_TEXT_X, ENEMY_HP_Y + 3, "", Theme::GOLD, 1, TR_DATUM),
      enemyAttackBar(BAR_X, ENEMY_ATK_BAR_Y, ENEMY_ATK_BAR_W, ENEMY_ATK_BAR_H, 0, 1, ENEMY_ATTACK_COLOR,
                     ENEMY_ATTACK_BG, ENEMY_ATTACK_BORDER),
      messageText(CENTER_X, MESSAGE_Y, "", Theme::TEXT, 1, TC_DATUM),
      actionPanel(combat),
      atkLabel(8, STATS_Y, "ATK", Theme::TEXT_2, 1),
      atkValue(28, STATS_Y, "", Theme::ATTACK, 1),
      defLabel(60, STATS_Y, "DEF", Theme::TEXT_2, 1),
      defValue(80, STATS_Y, "", Theme::DEFENSE, 1),
      playerHpBar(BAR_X, PLAYER_HP_Y, HP_BAR_W, PLAYER_HP_H, 0, 1, PLAYER_HP_COLOR, PLAYER_HP_BG, PLAYER_HP_BORDER),
      playerHitText(HIT_TEXT_X, PLAYER_HP_Y + 2, "", CombatStyle::BLOCKED, 1, TR_DATUM),
      combat(combat),
      sound(sound),
      seenKills(0),
      seenDrops(0),
      seenHits(0),
      seenHurts(0),
      messageUntil(0),
      enemyHitUntil(0),
      playerHitUntil(0),
      playerFlashUntil(0)
{
    addElement(&header);
    setHeader(&header);

    enemyPanel.addTo(*this);
    addElement(&enemyHpBar);
    addElement(&enemyHitText);
    addElement(&enemyAttackBar);
    addElement(&messageText);
    actionPanel.addTo(*this);
    addElement(&atkLabel);
    addElement(&atkValue);
    addElement(&defLabel);
    addElement(&defValue);
    addElement(&playerHpBar);
    addElement(&playerHitText);
}

void CombatScreen::onEnter(TFT_eSPI &tft)
{
    // Events from while the player was elsewhere must not replay their sounds or banners
    seenKills = combat.getKillEvents();
    seenDrops = combat.getDropEvents();
    seenHits = combat.getHitEvents();
    seenHurts = combat.getHurtEvents();
    messageUntil = 0;
    enemyHitUntil = 0;
    playerHitUntil = 0;
    playerFlashUntil = 0;
    forceRefresh();
    enemyPanel.hideBanner();
    Screen::onEnter(tft);
}

// Leaves out the elements whose visibility their own refresh decides (the hit numbers),
// which would otherwise be toggled twice per refresh and flicker
void CombatScreen::showFightElements(bool visible)
{
    enemyPanel.setVisible(visible);
    enemyHpBar.setVisible(visible);
    enemyAttackBar.setVisible(visible);
    actionPanel.setVisible(visible);
    atkLabel.setVisible(visible);
    atkValue.setVisible(visible);
    defLabel.setVisible(visible);
    defValue.setVisible(visible);
    playerHpBar.setVisible(visible);
}

void CombatScreen::showMessage(unsigned long now, const char *text, uint16_t color, unsigned long durationMs)
{
    messageText.setText(text);
    messageText.setColor(color);
    messageUntil = now + durationMs;
}

// Diffs the engine's monotonic counters against the last values this screen saw
void CombatScreen::handleEvents(unsigned long now)
{
    char buf[40];

    if (combat.getHitEvents() != seenHits)
    {
        seenHits = combat.getHitEvents();
        snprintf(buf, sizeof(buf), "-%u", (unsigned)combat.getLastHitDamage());
        enemyHitText.setText(buf);
        enemyHitUntil = now + HIT_TEXT_MS;
    }

    if (combat.getHurtEvents() != seenHurts)
    {
        seenHurts = combat.getHurtEvents();
        snprintf(buf, sizeof(buf), "-%u", (unsigned)combat.getLastHurtDamage());
        playerHitText.setText(buf);
        playerHitUntil = now + HIT_TEXT_MS;
        playerFlashUntil = now + HURT_FLASH_MS;
        sound.playHurt();
    }

    bool killed = combat.getKillEvents() != seenKills;
    if (killed)
    {
        seenKills = combat.getKillEvents();
        bool midas = combat.wasLastKillMidas();
        snprintf(buf, sizeof(buf), midas ? "MIDAS! +%s gold" : "Defeated! +%s gold",
                 formatAmount(combat.getLastKillGold()).c_str());
        showMessage(now, buf, midas ? Theme::GOLD : Theme::GOOD, KILL_MESSAGE_MS);
    }

    // A drop outranks the kill sound: it gets its own fanfare and the big card
    bool dropped = combat.getDropEvents() != seenDrops;
    if (dropped)
    {
        seenDrops = combat.getDropEvents();
        int8_t itemId = combat.getLastDrop();
        if (itemId != NO_ITEM)
        {
            bool isNew = combat.wasLastDropNew();
            enemyPanel.showDrop((uint8_t)itemId, isNew, combat.getLastDropLevel(),
                                now + (isNew ? NEW_DROP_MS : LEVEL_DROP_MS));
            if (isNew)
            {
                sound.playDrop();
            }
            else
            {
                sound.playBuy();
            }
        }
    }
    else if (killed)
    {
        sound.playLevelUp();
    }
}

// The enemy's next hit charges up rather than counting down, so the bar filling reads as
// "something is coming"; it turns red just before it lands
void CombatScreen::refreshEnemyAttack(unsigned long now, bool fighting)
{
    char buf[32];
    bool warn = false;

    if (!fighting)
    {
        enemyAttackBar.setProgress(0, 1);
        enemyAttackBar.setFillColor(ENEMY_ATTACK_COLOR);
        enemyAttackBar.setBorderColor(ENEMY_ATTACK_BORDER);
        enemyAttackBar.setLabel("");
    }
    else if (combat.getEnemyAttackDamage() == 0)
    {
        // A full static bar rather than an empty one: counting up to nothing read as a bug
        enemyAttackBar.setProgress(1, 1);
        enemyAttackBar.setFillColor(INERT_COLOR);
        enemyAttackBar.setBorderColor(ENEMY_ATTACK_BORDER);
        enemyAttackBar.setLabel("can't hurt you");
    }
    else
    {
        unsigned long interval = combat.getEnemy().attackIntervalMs;
        unsigned long left = combat.getEnemyAttackRemaining(now);
        uint32_t damage = combat.getEnemyHitDamage(now);
        // A stun pushed the hit more than one interval away: the bar waits empty until the
        // enemy starts winding up again. A ward makes the hit harmless, so no red warning.
        bool stunned = left > interval;
        bool warded = combat.isWarded(now);
        warn = !stunned && !warded && left < WARN_MS;

        enemyAttackBar.setProgress(stunned ? 0 : (int32_t)(interval - left), (int32_t)interval);
        enemyAttackBar.setFillColor(warn ? CombatStyle::BLOCKED : warded ? CombatStyle::ACTIVE : ENEMY_ATTACK_COLOR);
        enemyAttackBar.setBorderColor(warn ? CombatStyle::BLOCKED : warded ? CombatStyle::ACTIVE : ENEMY_ATTACK_BORDER);
        if (stunned)
        {
            enemyAttackBar.setLabel("stunned");
        }
        else if (warded)
        {
            enemyAttackBar.setLabel("warded - no damage");
        }
        else
        {
            snprintf(buf, sizeof(buf), "hits you for %u", (unsigned)damage);
            enemyAttackBar.setLabel(buf);
        }
    }
}

void CombatScreen::refreshPlayer(unsigned long now)
{
    char buf[24];
    const Stats &player = combat.getPlayerStats();

    snprintf(buf, sizeof(buf), "%u", (unsigned)player.attack);
    atkValue.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)player.defense);
    defValue.setText(buf);

    bool flash = (long)(now - playerFlashUntil) < 0;
    playerHpBar.setProgress((int32_t)combat.getPlayerHp(), (int32_t)player.maxHp);
    playerHpBar.setFillColor(flash ? CombatStyle::BLOCKED : PLAYER_HP_COLOR);
    playerHpBar.setBorderColor(flash ? CombatStyle::BLOCKED : PLAYER_HP_BORDER);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getPlayerHp(), (unsigned)player.maxHp);
    playerHpBar.setLabel(buf);

    playerHitText.setVisible((long)(now - playerHitUntil) < 0);
    enemyHitText.setVisible((long)(now - enemyHitUntil) < 0);
}

void CombatScreen::refreshStatus(unsigned long now, bool fighting)
{
    if (messageUntil != 0 && (long)(now - messageUntil) < 0)
    {
        return;
    }
    messageUntil = 0;
    if (combat.getPhase() == CombatState::DEAD)
    {
        char buf[40];
        unsigned long remaining = combat.getReviveRemaining(now);
        snprintf(buf, sizeof(buf), "Knocked out - back in %lu.%lus", remaining / 1000, (remaining % 1000) / 100);
        messageText.setColor(Theme::TEXT_2);
        messageText.setText(buf);
    }
    else if (fighting && combat.getBestAttackDamage() == 0)
    {
        messageText.setColor(CombatStyle::BLOCKED);
        messageText.setText("Too strong - improve your ATK");
    }
    else
    {
        messageText.setText("");
    }
}

void CombatScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }

    if (!combat.isInZone())
    {
        header.setTitle("Combat");
        showFightElements(false);
        enemyHitText.setVisible(false);
        playerHitText.setVisible(false);
        messageText.setColor(Theme::TEXT);
        messageText.setText("Pick a zone on the Zones screen");
        return;
    }

    handleEvents(now);

    const EnemyDef &enemy = combat.getEnemy();
    bool fighting = combat.getPhase() == CombatState::FIGHTING;

    header.setTitle(combat.getZone().name);
    showFightElements(true);
    enemyPanel.refresh(now);

    char buf[24];
    enemyHpBar.setProgress((int32_t)combat.getEnemyHp(), (int32_t)enemy.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getEnemyHp(), (unsigned)enemy.maxHp);
    enemyHpBar.setLabel(buf);

    refreshEnemyAttack(now, fighting);
    actionPanel.refresh(now, fighting);
    refreshPlayer(now);
    refreshStatus(now, fighting);
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
    forceRefresh();
}

// Casting says what it did in the status line, since a heal or a ward is easy to miss
void CombatScreen::onUpPress()
{
    const SpellDef *spell = combat.getSpell();
    uint32_t hpBefore = combat.getPlayerHp();
    if (spell == nullptr || !combat.castSpell())
    {
        sound.playError();
        forceRefresh();
        return;
    }

    sound.playLevelUp();
    unsigned long now = millis();
    char buf[32];
    switch (spell->effect)
    {
    case SpellEffect::HEAL:
        snprintf(buf, sizeof(buf), "Healed %u HP", (unsigned)(combat.getPlayerHp() - hpBefore));
        showMessage(now, buf, Theme::GOOD, KILL_MESSAGE_MS);
        break;
    case SpellEffect::STUN:
        showMessage(now, "Enemy stunned!", CombatStyle::ACTIVE, KILL_MESSAGE_MS);
        break;
    case SpellEffect::MIDAS:
        snprintf(buf, sizeof(buf), "Next kill pays x%u gold", (unsigned)spell->power);
        showMessage(now, buf, Theme::GOLD, KILL_MESSAGE_MS);
        break;
    case SpellEffect::WARD:
        snprintf(buf, sizeof(buf), "Warded for %us", (unsigned)(spell->power / 1000));
        showMessage(now, buf, CombatStyle::ACTIVE, KILL_MESSAGE_MS);
        break;
    }
    forceRefresh();
}

void CombatScreen::onDownPress()
{
    if (combat.guard())
    {
        sound.playMenu();
    }
    else
    {
        sound.playError();
    }
    forceRefresh();
}
