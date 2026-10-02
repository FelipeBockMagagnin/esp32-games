#include "CombatScreen.h"
#include "../game/GameConfig.h" // GOLD_COLOR
#include "../game/Inventory.h"

static const unsigned long REFRESH_MS = 100;

// How long a drop notification keeps the message line before the fight status takes it back
static const unsigned long DROP_MESSAGE_MS = 2500;

// Warm belongs to the enemy, cool to the player. Each bar gets a fill, a near-black tint
// of the same hue for the empty part, and a mid tone for the border; the fills are kept
// dark enough that the white label stays readable over them.
static const uint16_t ENEMY_HP_COLOR = 0xC8E3;   // Crimson
static const uint16_t ENEMY_HP_BG = 0x2800;
static const uint16_t ENEMY_HP_BORDER = 0x7800;
static const uint16_t ENEMY_ATTACK_COLOR = 0xCB20; // Burnt orange
static const uint16_t ENEMY_ATTACK_BG = 0x28A0;
static const uint16_t ENEMY_ATTACK_BORDER = 0x79E0;
static const uint16_t PLAYER_ATTACK_COLOR = 0x04B5; // Teal
static const uint16_t PLAYER_ATTACK_BG = 0x00E4;
static const uint16_t PLAYER_ATTACK_BORDER = 0x02CC;
static const uint16_t PLAYER_HP_COLOR = 0x2D45; // Leaf green
static const uint16_t PLAYER_HP_BG = 0x0100;
static const uint16_t PLAYER_HP_BORDER = 0x0320;
// Dark gray so the white bar label stays readable over a full, static bar
static const uint16_t INERT_COLOR = 0x39E7;

static const uint16_t READY_COLOR = 0x07E0;
static const uint16_t ACTIVE_COLOR = 0x07FF; // Cyan while guarding
static const uint16_t COOLING_COLOR = 0x7BEF; // Gray while on cooldown
static const uint16_t LOCKED_COLOR = 0x52AA;  // Dim gray when needs amulet
static const uint16_t ACTION_NAME_COLOR = 0xFFFF;
static const uint16_t DETAIL_COLOR = 0x9CF3;  // Mid gray: what the action does
static const uint16_t NEW_ITEM_COLOR = 0x07E0;     // Green for a first-time drop
static const uint16_t LEVEL_UP_COLOR = GOLD_COLOR; // A duplicate levelling the item up
static const uint16_t BLOCKED_COLOR = 0xF800;      // No attack can get through at all

static const uint16_t STAT_BOX_BORDER = 0x7800;   // Dark crimson border for enemy stat card
static const uint16_t STAT_BAND_COLOR = 0x5000;   // Its title strip, a shade darker
static const uint16_t SECTION_COLOR = 0xAD55;     // Card section titles
static const uint16_t DIVIDER_COLOR = 0x4000;
static const uint16_t ENEMY_NAME_COLOR = 0xFD4F;  // Warm peach, the enemy's side
static const uint16_t ENEMY_ATK_COLOR = 0xFB2C;   // Soft red
static const uint16_t ENEMY_DEF_COLOR = 0x6E7F;   // Steel blue
static const uint16_t ACTION_BOX_BORDER = 0x31A6; // Dark slate border for action buttons
static const uint16_t STATS_LINE_COLOR = 0x9E7F;  // Pale blue, the player's side

// Layout coordinates
static const int16_t CENTER_X = 120;
static const int16_t ENEMY_NAME_Y = 31;

// Enemy area: sprite on the left, stats card on the right
static const int16_t ENEMY_SPRITE_X = 10;
static const int16_t ENEMY_SPRITE_Y = 49;
static const int16_t ENEMY_SPRITE_SIZE = 128;

static const int16_t ENEMY_BOX_X = 142;
static const int16_t ENEMY_BOX_Y = 49;
static const int16_t ENEMY_BOX_W = 88;
static const int16_t ENEMY_BOX_H = 126;
static const int16_t ENEMY_BAND_H = 13;

// Drop grid inside the card: two columns of 16px icon + chance, three rows
static const int16_t DROP_GRID_Y = ENEMY_BOX_Y + 68;
static const int16_t DROP_CELL_W = 41;
static const int16_t DROP_CELL_H = 18;
static const int16_t DROP_ICON_SIZE = 16;
static const uint8_t DROP_COLUMNS = 2;

// Bars share one width so they stack as a single column
static const int16_t BAR_X = 15;
static const int16_t BAR_W = 210;
static const int16_t ENEMY_HP_Y = 180;
static const int16_t ENEMY_HP_H = 14;
static const int16_t ENEMY_ATK_BAR_Y = 196;
static const int16_t ENEMY_ATK_BAR_H = 12;

// Drop and fight status notifications
static const int16_t MESSAGE_Y = 211;

// Actions panel with button badges
static const int16_t ACTION_BOX_X = 15;
static const int16_t ACTION_BOX_Y = 228;
static const int16_t ACTION_BOX_W = 210;
static const int16_t ACTION_BOX_H = 38;

// Rows are 12px apart; text (8px) and badge (11px) are both centred on the row's middle
static const int16_t ACTION_BADGE_X = 20;
static const int16_t ACTION_LABEL_X = 35;
static const int16_t ACTION_DETAIL_X = 74;
static const int16_t ACTION_STATE_X = 220;
static const int16_t SMITE_ROW_Y = 231;
static const int16_t STRIKE_ROW_Y = 243;
static const int16_t GUARD_ROW_Y = 255;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2; // Relative to the text top

// Player area
static const int16_t PLAYER_ATK_BAR_Y = 268;
static const int16_t PLAYER_ATK_BAR_H = 12;
static const int16_t PLAYER_HP_Y = 283;
static const int16_t PLAYER_HP_H = 14;
static const int16_t PLAYER_STATS_Y = 301;

CombatScreen::CombatScreen(CombatState &combat, SoundManager &sound)
    : header("Combat", "Zones", "Mining"),

      // Enemy block
      enemyName(CENTER_X, ENEMY_NAME_Y, "", ENEMY_NAME_COLOR, 2, TC_DATUM),
      enemySprite(ENEMY_SPRITE_X, ENEMY_SPRITE_Y, ENEMY_SPRITE_SIZE, ENEMY_SPRITE_SIZE, image_rato_pixels),
      enemyStatsBox(ENEMY_BOX_X, ENEMY_BOX_Y, ENEMY_BOX_W, ENEMY_BOX_H, STAT_BOX_BORDER, false, 0x0000, true, 3),
      enemyStatsBand(ENEMY_BOX_X, ENEMY_BOX_Y, ENEMY_BOX_W, ENEMY_BAND_H, STAT_BOX_BORDER, true, STAT_BAND_COLOR, true, 3),
      enemyStatsHeader(ENEMY_BOX_X + ENEMY_BOX_W / 2, ENEMY_BOX_Y + 3, "STATS", 0xFFFF, 1, TC_DATUM),
      enemyAtkIcon(ENEMY_BOX_X + 6, ENEMY_BOX_Y + 18, 16, 16, image_sword_02b_pixels),
      enemyAtkText(ENEMY_BOX_X + 28, ENEMY_BOX_Y + 18, "", ENEMY_ATK_COLOR, 2),
      enemyDefIcon(ENEMY_BOX_X + 6, ENEMY_BOX_Y + 37, 16, 16, image_armor_01b_pixels),
      enemyDefText(ENEMY_BOX_X + 28, ENEMY_BOX_Y + 37, "", ENEMY_DEF_COLOR, 2),
      enemyDropsDivider(ENEMY_BOX_X + 4, ENEMY_BOX_Y + 56, ENEMY_BOX_W - 8, 1, DIVIDER_COLOR, true, DIVIDER_COLOR),
      enemyDropsLabel(ENEMY_BOX_X + ENEMY_BOX_W / 2, ENEMY_BOX_Y + 59, "DROPS", SECTION_COLOR, 1, TC_DATUM),

      enemyHpBar(BAR_X, ENEMY_HP_Y, BAR_W, ENEMY_HP_H, 0, 1, ENEMY_HP_COLOR, ENEMY_HP_BG, ENEMY_HP_BORDER),
      enemyAttackBar(BAR_X, ENEMY_ATK_BAR_Y, BAR_W, ENEMY_ATK_BAR_H, 0, 1, ENEMY_ATTACK_COLOR,
                     ENEMY_ATTACK_BG, ENEMY_ATTACK_BORDER),

      messageText(CENTER_X, MESSAGE_Y, "", 0xFFFF, 2, TC_DATUM),

      // Actions panel: Up smites (amulet), Confirm strikes, Down guards
      actionBox(ACTION_BOX_X, ACTION_BOX_Y, ACTION_BOX_W, ACTION_BOX_H, ACTION_BOX_BORDER, false, 0x0000, true, 3),
      smiteBadge(ACTION_BADGE_X, SMITE_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::UP),
      smiteLabel(ACTION_LABEL_X, SMITE_ROW_Y, "Smite", ACTION_NAME_COLOR, 1),
      smiteDetail(ACTION_DETAIL_X, SMITE_ROW_Y, "", DETAIL_COLOR, 1),
      smiteState(ACTION_STATE_X, SMITE_ROW_Y, "", READY_COLOR, 1, TR_DATUM),
      strikeBadge(ACTION_BADGE_X, STRIKE_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      strikeLabel(ACTION_LABEL_X, STRIKE_ROW_Y, "Strike", ACTION_NAME_COLOR, 1),
      strikeDetail(ACTION_DETAIL_X, STRIKE_ROW_Y, "", DETAIL_COLOR, 1),
      strikeState(ACTION_STATE_X, STRIKE_ROW_Y, "", READY_COLOR, 1, TR_DATUM),
      guardBadge(ACTION_BADGE_X, GUARD_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::DOWN),
      guardLabel(ACTION_LABEL_X, GUARD_ROW_Y, "Guard", ACTION_NAME_COLOR, 1),
      guardDetail(ACTION_DETAIL_X, GUARD_ROW_Y, "", DETAIL_COLOR, 1),
      guardState(ACTION_STATE_X, GUARD_ROW_Y, "", READY_COLOR, 1, TR_DATUM),

      // Player block
      playerAttackBar(BAR_X, PLAYER_ATK_BAR_Y, BAR_W, PLAYER_ATK_BAR_H, 0, 1, PLAYER_ATTACK_COLOR,
                      PLAYER_ATTACK_BG, PLAYER_ATTACK_BORDER),
      playerHpBar(BAR_X, PLAYER_HP_Y, BAR_W, PLAYER_HP_H, 0, 1, PLAYER_HP_COLOR, PLAYER_HP_BG, PLAYER_HP_BORDER),
      statsLine(CENTER_X, PLAYER_STATS_Y, "", STATS_LINE_COLOR, 1, TC_DATUM),

      combat(combat),
      sound(sound),
      lastRefresh(0),
      seenKills(0),
      seenDrops(0),
      messageUntil(0)
{
    addElement(&header);
    setHeader(&header);

    // Enemy card elements
    // The band is registered after the card so its fill covers the card's top corners,
    // and before the header text so the text lands on top of it
    addElement(&enemyStatsBox);
    addElement(&enemyStatsBand);
    addElement(&enemyStatsHeader);
    addElement(&enemyAtkIcon);
    addElement(&enemyAtkText);
    addElement(&enemyDefIcon);
    addElement(&enemyDefText);
    addElement(&enemyDropsDivider);
    addElement(&enemyDropsLabel);
    // Drop grid cells, row-major; hidden until update() fills them from the current enemy
    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        int16_t cellX = (int16_t)(ENEMY_BOX_X + 3 + (int16_t)(i % DROP_COLUMNS) * DROP_CELL_W);
        int16_t cellY = (int16_t)(DROP_GRID_Y + (int16_t)(i / DROP_COLUMNS) * DROP_CELL_H);
        dropIcon[i].setPosition(cellX, cellY);
        dropIcon[i].setVisible(false);
        addElement(&dropIcon[i]);

        // Vertically centred on the icon: 8px text beside a 16px icon
        dropPct[i].setPosition(cellX + DROP_ICON_SIZE + 2, cellY + 4);
        dropPct[i].setVisible(false);
        addElement(&dropPct[i]);
    }

    addElement(&enemyName);
    addElement(&enemySprite);
    addElement(&enemyHpBar);
    addElement(&enemyAttackBar);

    addElement(&messageText);

    // Action panel elements
    addElement(&actionBox);
    addElement(&smiteBadge);
    addElement(&smiteLabel);
    addElement(&smiteDetail);
    addElement(&smiteState);
    addElement(&strikeBadge);
    addElement(&strikeLabel);
    addElement(&strikeDetail);
    addElement(&strikeState);
    addElement(&guardBadge);
    addElement(&guardLabel);
    addElement(&guardDetail);
    addElement(&guardState);

    // Player elements
    addElement(&playerAttackBar);
    addElement(&playerHpBar);
    addElement(&statsLine);

    // Details are fixed by config, so they are written once: the hits show only their
    // multiplier and the guard only its reduction (its duration is a countdown in the
    // state column while it runs)
    char buf[24];
    snprintf(buf, sizeof(buf), "-%u%%", (unsigned)(100 - GUARD_DAMAGE_PERCENT));
    guardDetail.setText(buf);
    snprintf(buf, sizeof(buf), "x%u", (unsigned)STRIKE_MULTIPLIER);
    strikeDetail.setText(buf);
    snprintf(buf, sizeof(buf), "x%u", (unsigned)SMITE_MULTIPLIER);
    smiteDetail.setText(buf);
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
    enemyStatsBox.setVisible(visible);
    enemyStatsBand.setVisible(visible);
    enemyStatsHeader.setVisible(visible);
    enemyAtkIcon.setVisible(visible);
    enemyAtkText.setVisible(visible);
    enemyDefIcon.setVisible(visible);
    enemyDefText.setVisible(visible);
    enemyDropsDivider.setVisible(visible);
    enemyDropsLabel.setVisible(visible);
    if (!visible)
    {
        hideDrops();
    }
    enemyHpBar.setVisible(visible);

    actionBox.setVisible(visible);
    smiteBadge.setVisible(visible);
    smiteLabel.setVisible(visible);
    smiteDetail.setVisible(visible);
    smiteState.setVisible(visible);
    strikeBadge.setVisible(visible);
    strikeLabel.setVisible(visible);
    strikeDetail.setVisible(visible);
    strikeState.setVisible(visible);
    guardBadge.setVisible(visible);
    guardLabel.setVisible(visible);
    guardDetail.setVisible(visible);
    guardState.setVisible(visible);

    playerHpBar.setVisible(visible);
    statsLine.setVisible(visible);
}

void CombatScreen::hideDrops()
{
    for (uint8_t i = 0; i < MAX_DROPS; i++)
    {
        dropIcon[i].setVisible(false);
        dropPct[i].setVisible(false);
    }
}

void CombatScreen::refreshDrops(const EnemyDef &enemy)
{
    const Inventory &inventory = combat.getInventory();
    uint8_t count = (enemy.dropCount < MAX_DROPS) ? enemy.dropCount : MAX_DROPS;
    char buf[8];

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
        const ItemDef &def = ITEMS[drop.itemId];

        // Narrow icons are centred in the 16px cell. A 1-bit icon is drawn on an opaque
        // background so swapping enemies paints over the previous icon instead of
        // leaving its pixels behind (Image only erases when it moves or resizes)
        int16_t cellX = (int16_t)(ENEMY_BOX_X + 3 + (int16_t)(i % DROP_COLUMNS) * DROP_CELL_W);
        int16_t cellY = (int16_t)(DROP_GRID_Y + (int16_t)(i / DROP_COLUMNS) * DROP_CELL_H);
        dropIcon[i].setPosition(cellX + (DROP_ICON_SIZE - def.iconW) / 2, cellY + (DROP_ICON_SIZE - def.iconH) / 2);
        if (def.iconPixels != nullptr)
        {
            dropIcon[i].setPixels(def.iconPixels, def.iconW, def.iconH);
        }
        else
        {
            dropIcon[i].setBitmap(def.iconBitmap, def.iconW, def.iconH, 0xFFFF, false, 0x0000);
        }

        // Basis points: whole percents print plain, rare drops keep their tenth (1.2%, 0.6%)
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
        dropPct[i].setColor(inventory.isOwned(drop.itemId) ? LEVEL_UP_COLOR : NEW_ITEM_COLOR);
    }
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

    // A multiplier turns red when that hit cannot get through the enemy's defense
    strikeDetail.setColor(combat.getStrikeDamage() == 0 ? BLOCKED_COLOR : DETAIL_COLOR);

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
        // Counts down the time left on the guard, in the active color
        unsigned long left = combat.getGuardRemaining(now);
        snprintf(buf, sizeof(buf), "%lu.%lus", left / 1000, (left % 1000) / 100);
        guardState.setText(buf);
        guardState.setColor(ACTIVE_COLOR);
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
        smiteBadge.setDimmed(true);
        smiteLabel.setColor(LOCKED_COLOR);
        smiteDetail.setColor(LOCKED_COLOR);
        smiteState.setText("need amulet");
        smiteState.setColor(LOCKED_COLOR);
        return;
    }

    smiteBadge.setDimmed(false);
    smiteLabel.setColor(ACTION_NAME_COLOR);
    smiteDetail.setColor(combat.getSmiteDamage() == 0 ? BLOCKED_COLOR : DETAIL_COLOR);
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

    // Enemy stat card: ATK, DEF then drop pool
    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.attack);
    enemyAtkText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)enemy.defense);
    enemyDefText.setText(buf);

    refreshDrops(enemy);

    enemyHpBar.setProgress((int32_t)combat.getEnemyHp(), (int32_t)enemy.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getEnemyHp(), (unsigned)enemy.maxHp);
    enemyHpBar.setLabel(buf);

    playerHpBar.setProgress((int32_t)combat.getPlayerHp(), (int32_t)player.maxHp);
    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)combat.getPlayerHp(), (unsigned)player.maxHp);
    playerHpBar.setLabel(buf);

    // Footer: only player stats
    snprintf(buf, sizeof(buf), "ATK %u  DEF %u  HP %u",
             (unsigned)player.attack, (unsigned)player.defense, (unsigned)player.maxHp);
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

void CombatScreen::onUpPress()
{
    if (!combat.isSmiteUnlocked())
    {
        return;
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
