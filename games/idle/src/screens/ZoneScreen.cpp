#include "ZoneScreen.h"
#include "../game/Stats.h"

static const unsigned long REFRESH_MS = 100;

static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 40;
static const int16_t ROW_PITCH = 28;

static const int16_t FOOTER_ICON_Y = 296;
static const int16_t FOOTER_TEXT_Y = 297;

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

// Zones are fought from the front, so the toughest enemy sets expectations
static const EnemyDef &toughestEnemy(const ZoneDef &zone)
{
    uint8_t best = 0;
    for (uint8_t i = 1; i < zone.enemyCount; i++)
    {
        if (zone.enemies[i].attack > zone.enemies[best].attack)
        {
            best = i;
        }
    }
    return zone.enemies[best];
}

ZoneScreen::ZoneScreen(GameState &game, Inventory &inventory, CombatState &combat,
                       SoundManager &sound, ScreenManager &screens)
    : header("Zones", "Inventory", "Combat"),

      attackIcon(8, FOOTER_ICON_Y, 16, 16, image_sword_02b_pixels),
      attackText(28, FOOTER_TEXT_Y, "0", 0xFFFF, 2),
      defenseIcon(88, FOOTER_ICON_Y, 16, 16, image_armor_01b_pixels),
      defenseText(108, FOOTER_TEXT_Y, "0", 0xFFFF, 2),
      hpIcon(166, FOOTER_ICON_Y, 15, 16, image_cards_hearts_bits),
      hpText(185, FOOTER_TEXT_Y, "0", 0xFFFF, 2),

      game(game),
      inventory(inventory),
      combat(combat),
      sound(sound),
      screens(screens),
      lastRefresh(0)
{
    addElement(&header);
    setHeader(&header);

    for (uint8_t i = 0; i < ListView::VISIBLE_ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        addElement(&rows[i]);
    }

    addElement(&attackIcon);
    addElement(&attackText);
    addElement(&defenseIcon);
    addElement(&defenseText);
    addElement(&hpIcon);
    addElement(&hpText);

    list.setCount(ZONE_COUNT);
}

void ZoneScreen::update(unsigned long now)
{
    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

    char buf[40];
    uint8_t visible = list.getVisibleCount();

    for (uint8_t i = 0; i < ListView::VISIBLE_ROWS; i++)
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

        if (!unlocked)
        {
            if (zone.requiresUnlockUpgrade && !game.isZoneUnlockedByUpgrade(id))
            {
                snprintf(buf, sizeof(buf), "buy %s", unlockUpgradeName(id));
            }
            else
            {
                snprintf(buf, sizeof(buf), "%u kills in %s",
                         (unsigned)zone.reqPrevZoneKills, ZONES[id - 1].name);
            }
            row.setSubtitle(buf);
            row.setValue("");
            row.setValueSub("locked");
            row.setState(isSelected ? ListRow::SELECTED : ListRow::DIMMED);
            continue;
        }

        const EnemyDef &enemy = toughestEnemy(zone);
        snprintf(buf, sizeof(buf), "atk %u def %u hp %u",
                 (unsigned)enemy.attack, (unsigned)enemy.defense, (unsigned)enemy.maxHp);
        row.setSubtitle(buf);

        snprintf(buf, sizeof(buf), "%u", (unsigned)combat.getZoneKills(id));
        row.setValue(buf);
        row.setValueSub("kills");

        if (isSelected)
        {
            row.setState(ListRow::SELECTED);
        }
        else
        {
            row.setState(isActive ? ListRow::OWNED : ListRow::NORMAL);
        }
    }

    Stats total = resolvePlayerStats(game, inventory);
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.attack);
    attackText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.defense);
    defenseText.setText(buf);
    snprintf(buf, sizeof(buf), "%u", (unsigned)total.maxHp);
    hpText.setText(buf);
}

void ZoneScreen::onSelectPress()
{
    list.next();
    lastRefresh = 0;
}

void ZoneScreen::onConfirmPress()
{
    if (combat.enterZone((uint8_t)list.getSelected()))
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
