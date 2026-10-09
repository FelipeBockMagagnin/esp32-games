#include "ZoneScreen.h"
#include "../game/Format.h"
#include "../game/Ascension.h"


static const uint16_t CARD_BORDER = Theme::FRAME; // Dark slate, as on the Inventory stats card
static const uint16_t PANEL_BORDER = Theme::FRAME;
static const uint16_t ATTACK_COLOR = Theme::ATTACK;  // Soft red, as on the combat screen
static const uint16_t DEFENSE_COLOR = Theme::DEFENSE; // Steel blue
static const uint16_t HP_COLOR = Theme::HP;      // Leaf green
static const uint16_t HINT_COLOR = Theme::TEXT_2;
static const uint16_t STATS_COLOR = Theme::TEXT;
static const uint16_t ENEMY_NAME_COLOR = Theme::ENEMY_NAME; // Warm peach, the enemy's side on the combat screen
static const uint16_t ACTIVE_COLOR = Theme::GOOD;



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
    : header("Zones"),

      statsCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, true, Theme::CARD, true, 4),
      attackIcon(CARD_X + 8, STAT_ICON_Y, 16, 16, image_sword_02b_pixels, true, 0x0000),
      attackText(CARD_X + 28, STAT_TEXT_Y, "0", ATTACK_COLOR, 2),
      defenseIcon(CARD_X + 84, STAT_ICON_Y, 16, 16, image_armor_01b_pixels, true, 0x0000),
      defenseText(CARD_X + 104, STAT_TEXT_Y, "0", DEFENSE_COLOR, 2),
      hpIcon(CARD_X + 158, STAT_ICON_Y, 16, 16, image_upgrade_heart_pixels, true, 0x0000),
      hpText(CARD_X + 178, STAT_TEXT_Y, "0", HP_COLOR, 2),

      list(ROWS),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, true, Theme::CARD, true, 4),
      enemiesCaption(TEXT_X, CAPTION_Y, "ENEMIES", HINT_COLOR, 1),
      statusText(TEXT_RIGHT, CAPTION_Y, "", HINT_COLOR, 1, TR_DATUM),
      dropsCaption(TEXT_X, DROPS_CAPTION_Y, "DROPS", HINT_COLOR, 1),
      dropsFound(TEXT_RIGHT, DROPS_CAPTION_Y, "", HINT_COLOR, 1, TR_DATUM),
      dropIcons(TEXT_X, DROPS_Y),
      fightBadge(TEXT_X, FIGHT_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      fightLabel(TEXT_X + ButtonBadge::SIZE + 4, FIGHT_ROW_Y, "", Theme::TEXT, 1),
      rewardText(TEXT_RIGHT, FIGHT_ROW_Y, "", Theme::GOLD, 1, TR_DATUM),

      game(game),
      inventory(inventory),
      combat(combat),
      sound(sound),
      screens(screens)
{
    addElement(&header);
    setHeader(&header);

    addElement(&statsCard);
    addElement(&attackIcon, Theme::CARD);
    addElement(&attackText, Theme::CARD);
    addElement(&defenseIcon, Theme::CARD);
    addElement(&defenseText, Theme::CARD);
    addElement(&hpIcon, Theme::CARD);
    addElement(&hpText, Theme::CARD);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        addElement(&rows[i]);
    }

    addElement(&detailPanel);
    addElement(&enemiesCaption, Theme::CARD);
    addElement(&statusText, Theme::CARD);
    for (uint8_t i = 0; i < ENEMY_LINES; i++)
    {
        int16_t lineY = ENEMY_Y + i * ENEMY_PITCH;
        enemyName[i] = Text(TEXT_X, lineY, "", ENEMY_NAME_COLOR, 1);
        enemyStats[i] = Text(TEXT_RIGHT, lineY, "", STATS_COLOR, 1, TR_DATUM);
        enemyDeal[i] = Text(TEXT_X, lineY + ENEMY_LINE2_DY, "", Theme::TEXT, 1);
        addElement(&enemyName[i], Theme::CARD);
        addElement(&enemyStats[i], Theme::CARD);
        addElement(&enemyDeal[i], Theme::CARD);
    }
    addElement(&dropsCaption, Theme::CARD);
    addElement(&dropsFound, Theme::CARD);
    addElement(&dropIcons, Theme::CARD);
    addElement(&fightBadge, Theme::CARD);
    addElement(&fightLabel, Theme::CARD);
    addElement(&rewardText, Theme::CARD);

    list.setCount(ZONE_COUNT);
}

void ZoneScreen::refreshRows(const Stats &stats)
{
    char buf[40];
    uint8_t visible = list.getVisibleCount();

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
            else if (zone.reqDelveLevel > game.getAscension().getEffectLevel(TalentEffect::DEEP_ZONES))
            {
                snprintf(buf, sizeof(buf), "Deep Delver %u talent", (unsigned)zone.reqDelveLevel);
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
        row.setValue("");

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
    char buf[40];

    statusText.setText(isActive ? "Fighting here" : unlocked ? "" : "Locked");
    statusText.setColor(isActive ? ACTIVE_COLOR : HINT_COLOR);

    // A zone has more enemies than the panel has lines, so it lists the two ends of the
    // range: the easiest (lowest defense) and the toughest, which is the one that sets the
    // zone's label. Ties go to the earlier enemy for the easiest and the later (the boss)
    // for the toughest. Shown for locked zones too, as something to gear for.
    uint8_t easiest = 0;
    uint8_t toughest = 0;
    uint64_t minGold = zone.enemies[0].goldReward;
    uint64_t maxGold = minGold;
    for (uint8_t i = 1; i < zone.enemyCount; i++)
    {
        const EnemyDef &enemy = zone.enemies[i];
        if (enemy.defense < zone.enemies[easiest].defense)
        {
            easiest = i;
        }
        if (enemy.defense >= zone.enemies[toughest].defense)
        {
            toughest = i;
        }
        minGold = enemy.goldReward < minGold ? enemy.goldReward : minGold;
        maxGold = enemy.goldReward > maxGold ? enemy.goldReward : maxGold;
    }
    const uint8_t listed[ENEMY_LINES] = {easiest, toughest};
    uint8_t listedCount = zone.enemyCount > 1 ? ENEMY_LINES : 1;

    snprintf(buf, sizeof(buf), "%u ENEMIES", (unsigned)zone.enemyCount);
    enemiesCaption.setText(zone.enemyCount > 1 ? buf : "ENEMY");

    for (uint8_t i = 0; i < ENEMY_LINES; i++)
    {
        bool shown = i < listedCount;
        enemyName[i].setVisible(shown);
        enemyStats[i].setVisible(shown);
        enemyDeal[i].setVisible(shown);
        if (!shown)
        {
            continue;
        }

        const EnemyDef &enemy = zone.enemies[listed[i]];
        enemyName[i].setText(enemy.name);
        snprintf(buf, sizeof(buf), "HP %lu DEF %lu", (unsigned long)enemy.maxHp, (unsigned long)enemy.defense);
        enemyStats[i].setText(buf);

        uint32_t autoHit = damageThrough(stats.attack, enemy.defense);
        uint32_t strikeHit = damageThrough(stats.attack * STRIKE_MULTIPLIER, enemy.defense);
        snprintf(buf, sizeof(buf), "hit %lu / %lu", (unsigned long)autoHit, (unsigned long)strikeHit);
        enemyDeal[i].setText(buf);
        enemyDeal[i].setColor(Theme::TEXT);
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
            // A relic stays a secret until Relic Sense lets it drop
            if (!combat.canDrop(itemId))
            {
                continue;
            }
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
    fightLabel.setColor(unlocked ? Theme::TEXT : HINT_COLOR);

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
    feedback.update(now);
    if (!refreshDue(now))
    {
        return;
    }

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
    forceRefresh();
}

void ZoneScreen::onDownPress()
{
    list.next();
    forceRefresh();
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
        forceRefresh();
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
        feedback.shake(rows[id - list.getFirstVisible()]);
    }
    forceRefresh();
}
