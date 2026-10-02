#include "ZoneScreen.h"
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 100;

static const uint16_t CARD_BORDER = 0x31A6; // Dark slate, as on the Inventory stats card
static const uint16_t PANEL_BORDER = 0x31A6;
static const uint16_t ROW_FRAME = 0x31A6;
static const uint16_t SELECTED_FILL = 0x2124;
static const uint16_t ATTACK_COLOR = 0xFB2C;  // Soft red, as on the combat screen
static const uint16_t DEFENSE_COLOR = 0x6E7F; // Steel blue
static const uint16_t HP_COLOR = 0x2D45;      // Leaf green
static const uint16_t HINT_COLOR = 0x94B2;
static const uint16_t STATS_COLOR = 0xCE59;
static const uint16_t ENEMY_NAME_COLOR = 0xFD4F; // Warm peach, the enemy's side on the combat screen
static const uint16_t ACTIVE_COLOR = 0x07E0;

// How far the player's gear reaches into a zone, best to worst
enum Reach : uint8_t
{
    REACH_IDLE,    // The auto-attack gets through: the zone farms itself
    REACH_STRIKE,  // Only the manual strike gets through
    REACH_SMITE,   // Only the amulet's smite gets through
    REACH_BLOCKED  // Nothing gets through yet
};

static const char *const REACH_LABELS[] = {"IDLE", "STRIKE", "SMITE", "TOO HARD"};
static const uint16_t REACH_COLORS[] = {0x07E0, 0xFE60, 0xD3DF, 0xF8A6}; // Green, amber, purple, red

// Stats card
static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 31;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 30;
static const int16_t STAT_ICON_Y = CARD_Y + 7;
static const int16_t STAT_TEXT_Y = CARD_Y + 8;

// List
static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 65;
static const int16_t ROW_PITCH = 28;

// Detail panel; text (8px) and badge (11px) are centred on the row's middle
static const int16_t PANEL_X = 5;
static const int16_t PANEL_Y = 207;
static const int16_t PANEL_W = 230;
static const int16_t PANEL_H = 106;
static const int16_t TEXT_X = PANEL_X + 7;
static const int16_t TEXT_RIGHT = PANEL_X + PANEL_W - 7;
static const int16_t CAPTION_Y = PANEL_Y + 5;
static const int16_t ENEMY_Y = PANEL_Y + 17;
static const int16_t ENEMY_PITCH = 23;
static const int16_t ENEMY_LINE2_DY = 10;
static const int16_t DROPS_CAPTION_Y = PANEL_Y + 64;
static const int16_t DROPS_Y = PANEL_Y + 74;
static const int16_t FIGHT_ROW_Y = PANEL_Y + 94;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2;

// No floor on damage, as in CombatState::damageTo: an attack that cannot beat the
// defense does nothing
static uint32_t damageThrough(uint32_t attack, uint32_t defense)
{
    return attack > defense ? attack - defense : 0;
}

static Reach reachAgainst(const Stats &stats, const EnemyDef &enemy, bool smiteUnlocked)
{
    if (damageThrough(stats.attack, enemy.defense) > 0)
    {
        return REACH_IDLE;
    }
    if (damageThrough(stats.attack * STRIKE_MULTIPLIER, enemy.defense) > 0)
    {
        return REACH_STRIKE;
    }
    if (smiteUnlocked && damageThrough(stats.attack * SMITE_MULTIPLIER, enemy.defense) > 0)
    {
        return REACH_SMITE;
    }
    return REACH_BLOCKED;
}

// A zone is only as easy as its hardest enemy, since the fight picks between them
static Reach reachInZone(const Stats &stats, const ZoneDef &zone, bool smiteUnlocked)
{
    Reach worst = REACH_IDLE;
    for (uint8_t i = 0; i < zone.enemyCount; i++)
    {
        Reach reach = reachAgainst(stats, zone.enemies[i], smiteUnlocked);
        if (reach > worst)
        {
            worst = reach;
        }
    }
    return worst;
}

// The UNLOCK_ZONE upgrade for a zone carries its index, so its name can be looked up
// instead of duplicated in the zone table
static const char *unlockUpgradeName(uint8_t zoneId)
{
    for (uint8_t i = 0; i < GOLD_UPGRADE_COUNT; i++)
    {
        if (GOLD_UPGRADES[i].target == UpgradeTarget::UNLOCK_ZONE &&
            GOLD_UPGRADES[i].bonusPercent == zoneId)
        {
            return GOLD_UPGRADES[i].name;
        }
    }
    return "an upgrade";
}

ZoneScreen::ZoneScreen(GameState &game, Inventory &inventory, CombatState &combat,
                       SoundManager &sound, ScreenManager &screens)
    : header("Zones", "Inventory", "Combat"),

      statsCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, false, 0x0000, true, 4),
      attackIcon(CARD_X + 8, STAT_ICON_Y, 16, 16, image_sword_02b_pixels, true, 0x0000),
      attackText(CARD_X + 28, STAT_TEXT_Y, "0", ATTACK_COLOR, 2),
      defenseIcon(CARD_X + 84, STAT_ICON_Y, 16, 16, image_armor_01b_pixels, true, 0x0000),
      defenseText(CARD_X + 104, STAT_TEXT_Y, "0", DEFENSE_COLOR, 2),
      hpIcon(CARD_X + 158, STAT_ICON_Y, 16, 16, image_upgrade_heart_pixels, true, 0x0000),
      hpText(CARD_X + 178, STAT_TEXT_Y, "0", HP_COLOR, 2),

      list(ROWS),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, false, 0x0000, true, 4),
      enemiesCaption(TEXT_X, CAPTION_Y, "ENEMIES", HINT_COLOR, 1),
      statusText(TEXT_RIGHT, CAPTION_Y, "", HINT_COLOR, 1, TR_DATUM),
      dropsCaption(TEXT_X, DROPS_CAPTION_Y, "DROPS", HINT_COLOR, 1),
      dropsFound(TEXT_RIGHT, DROPS_CAPTION_Y, "", HINT_COLOR, 1, TR_DATUM),
      dropIcons(TEXT_X, DROPS_Y),
      fightBadge(TEXT_X, FIGHT_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      fightLabel(TEXT_X + ButtonBadge::SIZE + 4, FIGHT_ROW_Y, "", 0xFFFF, 1),
      rewardText(TEXT_RIGHT, FIGHT_ROW_Y, "", GOLD_COLOR, 1, TR_DATUM),

      game(game),
      inventory(inventory),
      combat(combat),
      sound(sound),
      screens(screens),
      lastRefresh(0)
{
    addElement(&header);
    setHeader(&header);

    addElement(&statsCard);
    addElement(&attackIcon);
    addElement(&attackText);
    addElement(&defenseIcon);
    addElement(&defenseText);
    addElement(&hpIcon);
    addElement(&hpText);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        rows[i].setFrameColor(ROW_FRAME);
        rows[i].setSelectedFill(SELECTED_FILL);
        addElement(&rows[i]);
    }

    addElement(&detailPanel);
    addElement(&enemiesCaption);
    addElement(&statusText);
    for (uint8_t i = 0; i < ENEMY_LINES; i++)
    {
        int16_t lineY = ENEMY_Y + i * ENEMY_PITCH;
        enemyName[i] = Text(TEXT_X, lineY, "", ENEMY_NAME_COLOR, 1);
        enemyStats[i] = Text(TEXT_RIGHT, lineY, "", STATS_COLOR, 1, TR_DATUM);
        enemyDeal[i] = Text(TEXT_X, lineY + ENEMY_LINE2_DY, "", 0xFFFF, 1);
        enemyHurt[i] = Text(TEXT_RIGHT, lineY + ENEMY_LINE2_DY, "", 0xFFFF, 1, TR_DATUM);
        addElement(&enemyName[i]);
        addElement(&enemyStats[i]);
        addElement(&enemyDeal[i]);
        addElement(&enemyHurt[i]);
    }
    addElement(&dropsCaption);
    addElement(&dropsFound);
    addElement(&dropIcons);
    addElement(&fightBadge);
    addElement(&fightLabel);
    addElement(&rewardText);

    list.setCount(ZONE_COUNT);
}

void ZoneScreen::refreshRows(const Stats &stats)
{
    char buf[40];
    uint8_t visible = list.getVisibleCount();
    bool smiteUnlocked = combat.isSmiteUnlocked();

    for (uint8_t i = 0; i < ROWS; i++)
    {
        ListRow &row = rows[i];
        if (i >= visible)
        {
            row.setVisible(false);
            continue;
        }

        uint8_t id = (uint8_t)(list.getFirstVisible() + i);
        const ZoneDef &zone = ZONES[id];
        bool unlocked = combat.isZoneUnlocked(id);
        bool isSelected = id == list.getSelected();
        bool isActive = combat.isInZone() && combat.getCurrentZone() == id;

        row.setVisible(true);
        row.setTitle(zone.name);
        row.setIconPixels(zone.icon, 16, 16);
        row.setValueSub("");

        if (!unlocked)
        {
            if (zone.requiresUnlockUpgrade && !game.isZoneUnlockedByUpgrade(id))
            {
                snprintf(buf, sizeof(buf), "buy %s", unlockUpgradeName(id));
            }
            else
            {
                snprintf(buf, sizeof(buf), "%u/%u kills in %s", (unsigned)combat.getZoneKills(id - 1),
                         (unsigned)zone.reqPrevZoneKills, ZONES[id - 1].name);
            }
            row.setSubtitle(buf);
            row.setValue("LOCKED");
            row.setState(isSelected ? ListRow::SELECTED : ListRow::DIMMED);
            continue;
        }

        snprintf(buf, sizeof(buf), isActive ? "%u kills, fighting" : "%u kills", (unsigned)combat.getZoneKills(id));
        row.setSubtitle(buf);

        Reach reach = reachInZone(stats, zone, smiteUnlocked);
        row.setValue(REACH_LABELS[reach]);
        row.setValueColor(REACH_COLORS[reach]);

        if (isSelected)
        {
            row.setState(ListRow::SELECTED);
        }
        else
        {
            row.setState(isActive ? ListRow::OWNED : ListRow::NORMAL);
        }
    }
}

void ZoneScreen::refreshPanel(const Stats &stats)
{
    uint8_t id = (uint8_t)list.getSelected();
    const ZoneDef &zone = ZONES[id];
    bool unlocked = combat.isZoneUnlocked(id);
    bool isActive = combat.isInZone() && combat.getCurrentZone() == id;
    bool smiteUnlocked = combat.isSmiteUnlocked();
    char buf[40];

    statusText.setText(isActive ? "Fighting here" : unlocked ? "" : "Locked");
    statusText.setColor(isActive ? ACTIVE_COLOR : HINT_COLOR);

    // Each enemy: its stats, then what each of the player's actions would deal to it
    // and what it would deal back. Shown for locked zones too, as something to gear for.
    uint32_t minGold = 0;
    uint32_t maxGold = 0;
    for (uint8_t i = 0; i < ENEMY_LINES; i++)
    {
        bool shown = i < zone.enemyCount;
        enemyName[i].setVisible(shown);
        enemyStats[i].setVisible(shown);
        enemyDeal[i].setVisible(shown);
        enemyHurt[i].setVisible(shown);
        if (!shown)
        {
            continue;
        }

        const EnemyDef &enemy = zone.enemies[i];
        enemyName[i].setText(enemy.name);
        snprintf(buf, sizeof(buf), "HP %lu DEF %lu", (unsigned long)enemy.maxHp, (unsigned long)enemy.defense);
        enemyStats[i].setText(buf);

        uint32_t autoHit = damageThrough(stats.attack, enemy.defense);
        uint32_t strikeHit = damageThrough(stats.attack * STRIKE_MULTIPLIER, enemy.defense);
        if (smiteUnlocked)
        {
            uint32_t smiteHit = damageThrough(stats.attack * SMITE_MULTIPLIER, enemy.defense);
            snprintf(buf, sizeof(buf), "hit %lu / %lu / %lu", (unsigned long)autoHit, (unsigned long)strikeHit,
                     (unsigned long)smiteHit);
        }
        else
        {
            snprintf(buf, sizeof(buf), "hit %lu / %lu", (unsigned long)autoHit, (unsigned long)strikeHit);
        }
        enemyDeal[i].setText(buf);
        enemyDeal[i].setColor(REACH_COLORS[reachAgainst(stats, enemy, smiteUnlocked)]);

        uint32_t hurt = damageThrough(enemy.attack, stats.defense);
        snprintf(buf, sizeof(buf), hurt > 0 ? "takes %lu" : "takes none", (unsigned long)hurt);
        enemyHurt[i].setText(buf);
        enemyHurt[i].setColor(hurt > 0 ? REACH_COLORS[REACH_BLOCKED] : REACH_COLORS[REACH_IDLE]);

        if (i == 0 || enemy.goldReward < minGold)
        {
            minGold = enemy.goldReward;
        }
        if (enemy.goldReward > maxGold)
        {
            maxGold = enemy.goldReward;
        }
    }

    // Every distinct item the zone can drop, grayed until found
    uint8_t dropCount = 0;
    uint8_t foundCount = 0;
    uint8_t seen[IconStrip::MAX_ICONS];
    for (uint8_t e = 0; e < zone.enemyCount; e++)
    {
        const EnemyDef &enemy = zone.enemies[e];
        for (uint8_t d = 0; d < enemy.dropCount && dropCount < IconStrip::MAX_ICONS; d++)
        {
            uint8_t itemId = enemy.drops[d].itemId;
            bool duplicate = false;
            for (uint8_t k = 0; k < dropCount; k++)
            {
                duplicate = duplicate || seen[k] == itemId;
            }
            if (duplicate)
            {
                continue;
            }
            bool owned = inventory.isOwned(itemId);
            seen[dropCount] = itemId;
            dropIcons.setIcon(dropCount, ITEMS[itemId].iconPixels, !owned);
            dropCount++;
            foundCount += owned ? 1 : 0;
        }
    }
    dropIcons.setCount(dropCount);
    snprintf(buf, sizeof(buf), "%u/%u found", (unsigned)foundCount, (unsigned)dropCount);
    dropsFound.setText(buf);

    fightBadge.setDimmed(!unlocked);
    fightLabel.setText(isActive ? "Resume fight" : unlocked ? "Fight here" : "Locked");
    fightLabel.setColor(unlocked ? 0xFFFF : HINT_COLOR);

    if (minGold == maxGold)
    {
        snprintf(buf, sizeof(buf), "%s gold/kill", formatAmount(maxGold).c_str());
    }
    else
    {
        snprintf(buf, sizeof(buf), "%s-%s gold/kill", formatAmount(minGold).c_str(), formatAmount(maxGold).c_str());
    }
    rewardText.setText(buf);
}

void ZoneScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    Stats total = resolvePlayerStats(game, inventory);
    char buf[12];
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.attack);
    attackText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.defense);
    defenseText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.maxHp);
    hpText.setText(buf);

    refreshRows(total);
    refreshPanel(total);
}

void ZoneScreen::onUpPress()
{
    list.previous();
    lastRefresh = 0;
}

void ZoneScreen::onDownPress()
{
    list.next();
    lastRefresh = 0;
}

void ZoneScreen::onConfirmPress()
{
    // Re-entering the zone already being fought would restart the fight (full HP, new
    // enemy), so for that one Confirm only goes back to the combat screen
    uint8_t id = (uint8_t)list.getSelected();
    if (combat.isInZone() && combat.getCurrentZone() == id)
    {
        sound.playMenu();
        screens.nextScreen();
        lastRefresh = 0;
        return;
    }

    if (combat.enterZone(id))
    {
        sound.playBuy();
        // Combat is the screen right after this one in the rotation
        screens.nextScreen();
    }
    else
    {
        sound.playError();
    }
    lastRefresh = 0;
}
