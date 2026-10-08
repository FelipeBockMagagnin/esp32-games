#include "CombatScreen.h"
#include "../game/GameConfig.h"
#include "../game/Inventory.h"
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 100;

// A new item holds the stage longer than a duplicate that only levelled one up
static const unsigned long NEW_DROP_MS = 3000;
static const unsigned long LEVEL_DROP_MS = 1800;
static const unsigned long KILL_MESSAGE_MS = 1500;

// A hit's damage stays beside its health bar this long; the player's bar flashes red for
// the first part of it, so a hit taken is noticed even when the eyes are elsewhere
static const unsigned long HIT_TEXT_MS = 800;
static const unsigned long HURT_FLASH_MS = 300;
// The enemy's attack bar turns red this long before it hits: enough to see it and press
// Down in time
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
// A full, static bar for an enemy that cannot hurt the player
static const uint16_t INERT_COLOR = Theme::INERT_BAR;

static const uint16_t READY_COLOR = Theme::SELECT; // Ring and outline of a pressable action
static const uint16_t ACTIVE_COLOR = Theme::INFO;  // Guard and ward while they are up
static const uint16_t AUTO_COLOR = Theme::ATTACK;  // The automatic attack's ring
static const uint16_t BLOCKED_COLOR = Theme::BAD;

// Layout coordinates
static const int16_t CENTER_X = 120;
static const int16_t ENEMY_NAME_Y = 31;

// The enemy alone in the middle, and the drop card spanning the width over it
static const int16_t ENEMY_SPRITE_X = (240 - EnemySprite::SIZE) / 2;
static const int16_t ENEMY_SPRITE_Y = 50;
static const int16_t BANNER_X = 6;
static const int16_t BANNER_W = 228;

// Enemy stats in the column left of the sprite, centred in it
static const int16_t STATS_CENTER_X = ENEMY_SPRITE_X / 2 + 1;
static const int16_t ENEMY_ATK_LABEL_Y = ENEMY_SPRITE_Y + 2;
static const int16_t ENEMY_ATK_VALUE_Y = ENEMY_ATK_LABEL_Y + 11;
static const int16_t ENEMY_DEF_LABEL_Y = ENEMY_ATK_LABEL_Y + 36;
static const int16_t ENEMY_DEF_VALUE_Y = ENEMY_DEF_LABEL_Y + 11;

// Drop list in the column right of the sprite: icon and chance per row
static const int16_t DROPS_X = ENEMY_SPRITE_X + EnemySprite::SIZE + 4;
static const int16_t DROPS_CENTER_X = (DROPS_X + 234) / 2;
static const int16_t DROPS_Y = ENEMY_SPRITE_Y + 2;
static const int16_t DROP_ROW_Y = DROPS_Y + 12;
static const int16_t DROP_PITCH = 19;
static const int16_t DROP_ICON_SIZE = 16;
static const int16_t DROP_PCT_DX = DROP_ICON_SIZE + 3;

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

// Actions: a column of buttons on the left, stacked like the device's Up / Confirm / Down
static const int16_t RING_CX = 18;
static const int16_t RING_R = 11;
static const int16_t ACTION_NAME_X = 35;
static const int16_t SPELL_CY = 237;
static const int16_t STRIKE_CY = 260;
static const int16_t GUARD_CY = 283;
static const int16_t NAME_DY = -8; // Name above the centre line, detail below
static const int16_t DETAIL_DY = 1;

// Automatic attack card on the right of the actions
static const int16_t AUTO_X = 128;
static const int16_t AUTO_Y = 225;
static const int16_t AUTO_W = 106;
static const int16_t AUTO_H = 70;
static const int16_t AUTO_CX = AUTO_X + AUTO_W / 2;
static const int16_t AUTO_RING_CY = AUTO_Y + 30;
static const int16_t AUTO_RING_R = 14;

// Player strip
static const int16_t STATS_Y = 298;
static const int16_t PLAYER_HP_Y = 308;
static const int16_t PLAYER_HP_H = 11;

#define ACTION_ROW(cy, kind, label)                                               \
    {                                                                             \
        CooldownRing(RING_CX, cy, RING_R, kind),                                  \
            Text(ACTION_NAME_X, (cy) + NAME_DY, label, Theme::TEXT_2, 1),         \
            Text(ACTION_NAME_X, (cy) + DETAIL_DY, "", Theme::TEXT_DIM, 1)         \
    }

CombatScreen::CombatScreen(CombatState &combat, SoundManager &sound)
    : header("Combat"),

      // Enemy block
      enemyName(CENTER_X, ENEMY_NAME_Y, "", Theme::ENEMY_NAME, 2, TC_DATUM),
      enemySprite(ENEMY_SPRITE_X, ENEMY_SPRITE_Y),
      dropBanner(BANNER_X, ENEMY_SPRITE_Y, BANNER_W, EnemySprite::SIZE,
                 ENEMY_SPRITE_X, ENEMY_SPRITE_Y, EnemySprite::SIZE, EnemySprite::SIZE),
      enemyAtkLabel(STATS_CENTER_X, ENEMY_ATK_LABEL_Y, "ATK", Theme::TEXT_DIM, 1, TC_DATUM),
      enemyAtkValue(STATS_CENTER_X, ENEMY_ATK_VALUE_Y, "", Theme::ATTACK, 2, TC_DATUM),
      enemyDefLabel(STATS_CENTER_X, ENEMY_DEF_LABEL_Y, "DEF", Theme::TEXT_DIM, 1, TC_DATUM),
      enemyDefValue(STATS_CENTER_X, ENEMY_DEF_VALUE_Y, "", Theme::DEFENSE, 2, TC_DATUM),
      dropsCaption(DROPS_CENTER_X, DROPS_Y, "DROPS", Theme::TEXT_DIM, 1, TC_DATUM),

      enemyHpBar(BAR_X, ENEMY_HP_Y, HP_BAR_W, ENEMY_HP_H, 0, 1, ENEMY_HP_COLOR, ENEMY_HP_BG, ENEMY_HP_BORDER),
      enemyHitText(HIT_TEXT_X, ENEMY_HP_Y + 3, "", Theme::GOLD, 1, TR_DATUM),
      enemyAttackBar(BAR_X, ENEMY_ATK_BAR_Y, ENEMY_ATK_BAR_W, ENEMY_ATK_BAR_H, 0, 1, ENEMY_ATTACK_COLOR,
                     ENEMY_ATTACK_BG, ENEMY_ATTACK_BORDER),

      messageText(CENTER_X, MESSAGE_Y, "", Theme::TEXT, 1, TC_DATUM),

      // Up casts the amulet's spell, Confirm strikes, Down guards
      actions{ACTION_ROW(SPELL_CY, ButtonBadge::UP, "SPELL"),
              ACTION_ROW(STRIKE_CY, ButtonBadge::CONFIRM, "STRIKE"),
              ACTION_ROW(GUARD_CY, ButtonBadge::DOWN, "GUARD")},

      autoCard(AUTO_X, AUTO_Y, AUTO_W, AUTO_H, Theme::FRAME, true, Theme::CARD, true, 4),
      autoTitle(AUTO_CX, AUTO_Y + 5, "AUTO", Theme::TEXT_2, 1, TC_DATUM),
      autoRing(AUTO_CX, AUTO_RING_CY, AUTO_RING_R),
      autoIcon(AUTO_CX - 8, AUTO_RING_CY - 8, 16, 16, image_sword_02b_pixels, true, 0x0000),
      autoHit(AUTO_CX, AUTO_Y + 50, "", Theme::TEXT, 1, TC_DATUM),
      autoNote(AUTO_CX, AUTO_Y + 60, "", Theme::TEXT_2, 1, TC_DATUM),

      // Player block
      atkLabel(8, STATS_Y, "ATK", Theme::TEXT_2, 1),
      atkValue(28, STATS_Y, "", Theme::ATTACK, 1),
      defLabel(60, STATS_Y, "DEF", Theme::TEXT_2, 1),
      defValue(80, STATS_Y, "", Theme::DEFENSE, 1),
      playerHpBar(BAR_X, PLAYER_HP_Y, HP_BAR_W, PLAYER_HP_H, 0, 1, PLAYER_HP_COLOR, PLAYER_HP_BG, PLAYER_HP_BORDER),
      playerHitText(HIT_TEXT_X, PLAYER_HP_Y + 2, "", BLOCKED_COLOR, 1, TR_DATUM),

      combat(combat),
      sound(sound),
      lastRefresh(0),
      seenKills(0),
      seenDrops(0),
      seenHits(0),
      seenHurts(0),
      messageUntil(0),
      dropUntil(0),
      enemyHitUntil(0),
      playerHitUntil(0),
      playerFlashUntil(0)
{
    addElement(&header);
    setHeader(&header);

    // The banner comes after the sprite so it lands on top of it, and the side columns
    // after the banner so they are redrawn on top of the area it clears when it goes
    addElement(&enemyName);
    addElement(&enemySprite);
    addElement(&dropBanner);
    addElement(&enemyAtkLabel);
    addElement(&enemyAtkValue);
    addElement(&enemyDefLabel);
    addElement(&enemyDefValue);
    addElement(&dropsCaption);
    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        int16_t rowY = DROP_ROW_Y + i * DROP_PITCH;
        dropIcon[i].setPosition(DROPS_X, rowY);
        dropIcon[i].setVisible(false);
        addElement(&dropIcon[i]);
        // Vertically centred on the icon: 8px text beside a 16px icon
        dropPct[i].setPosition(DROPS_X + DROP_PCT_DX, rowY + 4);
        dropPct[i].setVisible(false);
        addElement(&dropPct[i]);
    }
    addElement(&enemyHpBar);
    addElement(&enemyHitText);
    addElement(&enemyAttackBar);

    addElement(&messageText);

    for (uint8_t i = 0; i < ACTION_COUNT; i++)
    {
        addElement(&actions[i].ring);
        addElement(&actions[i].name);
        addElement(&actions[i].detail);
    }

    // The icon is registered after the ring so it lands inside it
    addElement(&autoCard);
    addElement(&autoTitle, Theme::CARD);
    addElement(&autoRing, Theme::CARD);
    addElement(&autoIcon, Theme::CARD);
    addElement(&autoHit, Theme::CARD);
    addElement(&autoNote, Theme::CARD);

    addElement(&atkLabel);
    addElement(&atkValue);
    addElement(&defLabel);
    addElement(&defValue);
    addElement(&playerHpBar);
    addElement(&playerHitText);
}

#undef ACTION_ROW

void CombatScreen::onEnter(TFT_eSPI &tft)
{
    // Events that happened while the player was elsewhere are already banked; resync here
    // so the screen does not replay their sounds or banners on arrival
    seenKills = combat.getKillEvents();
    seenDrops = combat.getDropEvents();
    seenHits = combat.getHitEvents();
    seenHurts = combat.getHurtEvents();
    messageUntil = 0;
    dropUntil = 0;
    enemyHitUntil = 0;
    playerHitUntil = 0;
    playerFlashUntil = 0;
    lastRefresh = 0;
    dropBanner.setVisible(false);
    Screen::onEnter(tft);
}

// Everything but the elements whose visibility their own refresh decides (the hit
// numbers, the drop card and list, and the auto attack ring), which would otherwise be
// toggled twice per refresh and flicker. The drop list is only hidden here.
void CombatScreen::showFightElements(bool visible)
{
    enemyName.setVisible(visible);
    enemySprite.setVisible(visible);
    enemyAtkLabel.setVisible(visible);
    enemyAtkValue.setVisible(visible);
    enemyDefLabel.setVisible(visible);
    enemyDefValue.setVisible(visible);
    dropsCaption.setVisible(visible);
    if (!visible)
    {
        for (uint8_t i = 0; i < MAX_DROPS; i++)
        {
            dropIcon[i].setVisible(false);
            dropPct[i].setVisible(false);
        }
    }
    enemyHpBar.setVisible(visible);
    enemyAttackBar.setVisible(visible);

    for (uint8_t i = 0; i < ACTION_COUNT; i++)
    {
        actions[i].ring.setVisible(visible);
        actions[i].name.setVisible(visible);
        actions[i].detail.setVisible(visible);
    }

    autoCard.setVisible(visible);
    autoTitle.setVisible(visible);
    autoHit.setVisible(visible);
    autoNote.setVisible(visible);

    atkLabel.setVisible(visible);
    atkValue.setVisible(visible);
    defLabel.setVisible(visible);
    defValue.setVisible(visible);
    playerHpBar.setVisible(visible);
}

// The side columns: the enemy's stats on the left and its drops on the right
void CombatScreen::refreshDrops(const EnemyDef &enemy)
{
    const Inventory &inventory = combat.getInventory();
    uint8_t count = enemy.dropCount < MAX_DROPS ? enemy.dropCount : MAX_DROPS;
    char buf[8];

    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.attack);
    enemyAtkValue.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.defense);
    enemyDefValue.setText(buf);

    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        bool used = i < count;
        dropIcon[i].setVisible(used);
        dropPct[i].setVisible(used);
        if (!used)
        {
            continue;
        }

        const DropDef &drop = enemy.drops[i];
        dropIcon[i].setPixels(ITEMS[drop.itemId].iconPixels, DROP_ICON_SIZE, DROP_ICON_SIZE, true, 0x0000);

        // Basis points: whole percents print plain, rare drops keep their tenth (5.5%)
        unsigned whole = drop.chanceBp / 100;
        unsigned tenth = (drop.chanceBp % 100) / 10;
        if (tenth == 0)
        {
            snprintf(buf, sizeof(buf), "%u%%", whole);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%u.%u%%", whole, tenth);
        }
        dropPct[i].setText(buf);
        dropPct[i].setColor(inventory.isOwned(drop.itemId) ? Theme::GOLD : Theme::GOOD);
    }
}

// The banner cleared both side columns when it went, so everything in them draws again
void CombatScreen::redrawSideColumns()
{
    enemyAtkLabel.resetDrawn();
    enemyAtkValue.resetDrawn();
    enemyDefLabel.resetDrawn();
    enemyDefValue.resetDrawn();
    dropsCaption.resetDrawn();
    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        dropIcon[i].resetDrawn();
        dropPct[i].resetDrawn();
    }
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
            dropBanner.show((uint8_t)itemId, isNew, combat.getLastDropLevel());
            dropUntil = now + (isNew ? NEW_DROP_MS : LEVEL_DROP_MS);
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

// The enemy, its stats and its drops, unless the drop card is covering them. When the card
// goes away the sprite and both side columns are redrawn whole, since the card only clears its own
// area outside the sprite.
void CombatScreen::refreshEnemy(unsigned long now)
{
    const EnemyDef &enemy = combat.getEnemy();
    enemyName.setText(enemy.name);

    if (dropBanner.isVisible())
    {
        if ((long)(now - dropUntil) < 0)
        {
            return;
        }
        dropBanner.setVisible(false);
        enemySprite.setSprite(enemy.sprite);
        enemySprite.setBackground(combat.getZone().background);
        enemySprite.resetDrawn();
        refreshDrops(enemy);
        redrawSideColumns();
        return;
    }

    enemySprite.setSprite(enemy.sprite);
    enemySprite.setBackground(combat.getZone().background);
    refreshDrops(enemy);
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
        uint32_t damage = combat.getEnemyAttackDamage();
        if (combat.isGuarding(now))
        {
            damage = damage * GUARD_DAMAGE_PERCENT / 100;
        }
        // A stun pushed the hit more than one interval away: the bar waits empty until the
        // enemy starts winding up again. A ward makes the hit harmless, so no red warning.
        bool stunned = left > interval;
        bool warded = combat.isWarded(now);
        warn = !stunned && !warded && left < WARN_MS;

        enemyAttackBar.setProgress(stunned ? 0 : (int32_t)(interval - left), (int32_t)interval);
        enemyAttackBar.setFillColor(warn ? BLOCKED_COLOR : warded ? ACTIVE_COLOR : ENEMY_ATTACK_COLOR);
        enemyAttackBar.setBorderColor(warn ? BLOCKED_COLOR : warded ? ACTIVE_COLOR : ENEMY_ATTACK_BORDER);
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

// One cooldown ring and the line under its name. Ready: gold ring and outline, and the
// hit it would deal. Recharging: a gray ring filling up and the time left. A hit that
// cannot beat the enemy's defense says so instead of inviting a useless press.
void CombatScreen::refreshCooldown(ActionRow &row, unsigned long left, unsigned long total, uint32_t damage,
                                   bool fighting)
{
    char buf[20];
    bool blocked = fighting && damage == 0;
    bool ready = fighting && !blocked && left == 0;

    // Cooldowns keep running between enemies, so the ring shows them even then
    row.ring.setProgress(left == 0 ? 1000 : (uint16_t)(1000 - left * 1000 / total));
    row.ring.setRingColor(ready ? READY_COLOR : Theme::TEXT_DIM);
    row.ring.setOutline(ready);
    row.ring.setCapDimmed(!fighting || blocked);
    row.name.setColor(ready ? Theme::TEXT : Theme::TEXT_2);

    if (blocked)
    {
        row.detail.setText("blocked");
        row.detail.setColor(BLOCKED_COLOR);
    }
    else if (left > 0)
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", left / 1000, (left % 1000) / 100);
        row.detail.setText(buf);
        row.detail.setColor(Theme::TEXT_DIM);
    }
    else
    {
        snprintf(buf, sizeof(buf), "hits %u", (unsigned)damage);
        row.detail.setText(fighting ? buf : "");
        row.detail.setColor(READY_COLOR);
    }
}

void CombatScreen::refreshActions(unsigned long now, bool fighting)
{
    refreshCooldown(actions[STRIKE], combat.getStrikeCooldownRemaining(now), STRIKE_COOLDOWN_MS,
                    combat.getStrikeDamage(), fighting);

    refreshSpell(now, fighting);

    // The guard deals no damage; while it is up its ring and line turn cyan and count the
    // time it has left
    ActionRow &guard = actions[GUARD];
    char buf[20];
    unsigned long left = combat.getGuardCooldownRemaining(now);
    bool guarding = combat.isGuarding(now);
    bool ready = fighting && left == 0;
    guard.ring.setProgress(left == 0 ? 1000 : (uint16_t)(1000 - left * 1000 / GUARD_COOLDOWN_MS));
    guard.ring.setRingColor(guarding ? ACTIVE_COLOR : ready ? READY_COLOR : Theme::TEXT_DIM);
    guard.ring.setOutline(ready);
    guard.ring.setCapDimmed(!fighting);
    guard.name.setColor(ready || guarding ? Theme::TEXT : Theme::TEXT_2);
    if (guarding)
    {
        unsigned long up = combat.getGuardRemaining(now);
        snprintf(buf, sizeof(buf), "on %lu.%lus", up / 1000, (up % 1000) / 100);
        guard.detail.setText(buf);
        guard.detail.setColor(ACTIVE_COLOR);
    }
    else if (left > 0)
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", left / 1000, (left % 1000) / 100);
        guard.detail.setText(buf);
        guard.detail.setColor(Theme::TEXT_DIM);
    }
    else
    {
        snprintf(buf, sizeof(buf), "-%u%% dmg", (unsigned)(100 - GUARD_DAMAGE_PERCENT));
        guard.detail.setText(fighting ? buf : "");
        guard.detail.setColor(READY_COLOR);
    }
}

// The Up button casts whatever the equipped amulet grants, so the row takes the spell's
// name. Under it: what the spell does when ready, the time left while recharging, or why
// it would do nothing now (full health for a heal, a Midas already armed). Without an
// amulet the row stays, dimmed, so the reward is discoverable.
void CombatScreen::refreshSpell(unsigned long now, bool fighting)
{
    ActionRow &row = actions[SPELL];
    const SpellDef *spell = combat.getSpell();
    char buf[20];

    if (spell == nullptr)
    {
        row.name.setText("SPELL");
        row.name.setColor(Theme::TEXT_DIM);
        row.ring.setProgress(0);
        row.ring.setRingColor(Theme::TEXT_DIM);
        row.ring.setOutline(false);
        row.ring.setCapDimmed(true);
        row.detail.setText("equip amulet");
        row.detail.setColor(Theme::TEXT_DIM);
        return;
    }

    unsigned long left = combat.getSpellCooldownRemaining(now);
    bool warded = combat.isWarded(now);
    bool useless = combat.isSpellUseless();
    bool ready = fighting && left == 0 && !useless;

    row.name.setText(spell->name);
    row.name.setColor(ready || warded ? Theme::TEXT : Theme::TEXT_2);
    row.ring.setProgress(left == 0 ? 1000 : (uint16_t)(1000 - left * 1000 / spell->cooldownMs));
    row.ring.setRingColor(warded ? ACTIVE_COLOR : ready ? READY_COLOR : Theme::TEXT_DIM);
    row.ring.setOutline(ready);
    row.ring.setCapDimmed(!fighting);

    if (warded)
    {
        unsigned long up = combat.getWardRemaining(now);
        snprintf(buf, sizeof(buf), "on %lu.%lus", up / 1000, (up % 1000) / 100);
        row.detail.setText(buf);
        row.detail.setColor(ACTIVE_COLOR);
        return;
    }
    if (combat.isMidasArmed())
    {
        snprintf(buf, sizeof(buf), "next kill x%u", (unsigned)spell->power);
        row.detail.setText(buf);
        row.detail.setColor(Theme::GOLD);
        return;
    }
    if (left > 0)
    {
        snprintf(buf, sizeof(buf), "%lu.%lus", left / 1000, (left % 1000) / 100);
        row.detail.setText(buf);
        row.detail.setColor(Theme::TEXT_DIM);
        return;
    }
    if (useless)
    {
        row.detail.setText("full HP");
        row.detail.setColor(Theme::TEXT_DIM);
        return;
    }

    switch (spell->effect)
    {
    case SpellEffect::HEAL:
        snprintf(buf, sizeof(buf), "heal %u",
                 (unsigned)(combat.getPlayerStats().maxHp * spell->power / 100));
        break;
    case SpellEffect::STUN:
        snprintf(buf, sizeof(buf), "skip a hit");
        break;
    case SpellEffect::MIDAS:
        snprintf(buf, sizeof(buf), "x%u gold", (unsigned)spell->power);
        break;
    case SpellEffect::WARD:
        snprintf(buf, sizeof(buf), "block %us", (unsigned)(spell->power / 1000));
        break;
    }
    row.detail.setText(fighting ? buf : "");
    row.detail.setColor(READY_COLOR);
}

// The automatic attack fills its own ring every AUTO_ATTACK_MS. When the enemy's defense
// blocks it, the ring goes away and the card points at the strike instead.
void CombatScreen::refreshAutoAttack(unsigned long now, bool fighting)
{
    char buf[20];
    uint32_t damage = combat.getAutoAttackDamage();
    bool working = damage > 0;

    autoRing.setVisible(working);
    autoIcon.setVisible(working);
    if (working)
    {
        unsigned long left = combat.getAutoAttackRemaining(now);
        autoRing.setProgress(fighting ? (uint16_t)(1000 - left * 1000 / AUTO_ATTACK_MS) : 0);
        autoRing.setRingColor(AUTO_COLOR);
        snprintf(buf, sizeof(buf), "hits %u", (unsigned)damage);
        autoHit.setText(buf);
        autoHit.setColor(Theme::TEXT);
        autoNote.setText("");
    }
    else
    {
        autoHit.setText("blocked");
        autoHit.setColor(BLOCKED_COLOR);
        autoNote.setText("use STRIKE");
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
    playerHpBar.setFillColor(flash ? BLOCKED_COLOR : PLAYER_HP_COLOR);
    playerHpBar.setBorderColor(flash ? BLOCKED_COLOR : PLAYER_HP_BORDER);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getPlayerHp(), (unsigned)player.maxHp);
    playerHpBar.setLabel(buf);

    playerHitText.setVisible((long)(now - playerHitUntil) < 0);
    enemyHitText.setVisible((long)(now - enemyHitUntil) < 0);
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
        dropBanner.setVisible(false);
        enemyHitText.setVisible(false);
        playerHitText.setVisible(false);
        autoRing.setVisible(false);
        autoIcon.setVisible(false);
        messageText.setColor(Theme::TEXT);
        messageText.setText("Pick a zone on the Zones screen");
        return;
    }

    handleEvents(now);

    const EnemyDef &enemy = combat.getEnemy();
    bool fighting = combat.getPhase() == CombatState::FIGHTING;

    header.setTitle(combat.getZone().name);
    showFightElements(true);
    refreshEnemy(now);

    enemyHpBar.setProgress((int32_t)combat.getEnemyHp(), (int32_t)enemy.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getEnemyHp(), (unsigned)enemy.maxHp);
    enemyHpBar.setLabel(buf);

    refreshEnemyAttack(now, fighting);
    refreshActions(now, fighting);
    refreshAutoAttack(now, fighting);
    refreshPlayer(now);

    // With no news to show, the status line carries the fight's standing state
    if (messageUntil == 0 || (long)(now - messageUntil) >= 0)
    {
        messageUntil = 0;
        if (combat.getPhase() == CombatState::DEAD)
        {
            unsigned long remaining = combat.getReviveRemaining(now);
            snprintf(buf, sizeof(buf), "Knocked out - back in %lu.%lus", remaining / 1000, (remaining % 1000) / 100);
            messageText.setColor(Theme::TEXT_2);
            messageText.setText(buf);
        }
        else if (fighting && combat.getBestAttackDamage() == 0)
        {
            // No action gets through at all, so the zone itself is out of reach
            messageText.setColor(BLOCKED_COLOR);
            messageText.setText("Too strong - improve your ATK");
        }
        else
        {
            messageText.setText("");
        }
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

// Casting says what it did in the status line, since a heal or a ward is easy to miss
void CombatScreen::onUpPress()
{
    const SpellDef *spell = combat.getSpell();
    uint32_t hpBefore = combat.getPlayerHp();
    if (spell == nullptr || !combat.castSpell())
    {
        sound.playError();
        lastRefresh = 0;
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
        showMessage(now, "Enemy stunned!", ACTIVE_COLOR, KILL_MESSAGE_MS);
        break;
    case SpellEffect::MIDAS:
        snprintf(buf, sizeof(buf), "Next kill pays x%u gold", (unsigned)spell->power);
        showMessage(now, buf, Theme::GOLD, KILL_MESSAGE_MS);
        break;
    case SpellEffect::WARD:
        snprintf(buf, sizeof(buf), "Warded for %us", (unsigned)(spell->power / 1000));
        showMessage(now, buf, ACTIVE_COLOR, KILL_MESSAGE_MS);
        break;
    }
    lastRefresh = 0;
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
    lastRefresh = 0;
}
