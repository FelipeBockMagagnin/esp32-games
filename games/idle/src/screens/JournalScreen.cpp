#include "JournalScreen.h"
#include "../game/Format.h"
#include "../game/Stats.h"

static constexpr Color HEADING_COLOR = Theme::GOLD;
static constexpr Color LABEL_COLOR = Theme::TEXT_2;
static constexpr Color TAB_ON = Theme::SELECT;
static constexpr Color TAB_OFF = Theme::TEXT_DIM;

static const int16_t TAB_Y = 34;
static const int16_t BODY_Y = 47;
static const int16_t LINE_X = 10;
static const int16_t LINE_RIGHT = 230;
static const int16_t LINE_Y = 51;
static const int16_t LINE_PITCH = 13;

static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 50;
static const int16_t ROW_PITCH = 28;
static const int16_t PANEL_X = 5;
static const int16_t PANEL_Y = 248;
static const int16_t PANEL_W = 230;
static const int16_t PANEL_H = 68;
static const int16_t PANEL_TEXT_X = PANEL_X + 7;

static uint16_t enemyTotal()
{
    uint16_t total = 0;
    for (uint8_t z = 0; z < ZONE_COUNT; z++)
    {
        total += ZONES[z].enemyCount;
    }
    return total;
}

// "45s", "12m 05s", "3h 07m", "2d 04h"
static void formatDuration(uint32_t seconds, char *buf, size_t size)
{
    if (seconds < 60)
    {
        snprintf(buf, size, "%lus", (unsigned long)seconds);
    }
    else if (seconds < 3600)
    {
        snprintf(buf, size, "%lum %02lus", (unsigned long)(seconds / 60), (unsigned long)(seconds % 60));
    }
    else if (seconds < 86400)
    {
        snprintf(buf, size, "%luh %02lum", (unsigned long)(seconds / 3600), (unsigned long)(seconds % 3600 / 60));
    }
    else
    {
        snprintf(buf, size, "%lud %02luh", (unsigned long)(seconds / 86400), (unsigned long)(seconds % 86400 / 3600));
    }
}

JournalScreen::JournalScreen(const GameState &game, const Inventory &inventory, const CombatState &combat,
                             const Achievements &achievements, const Ascension &ascension, const Journal &journal,
                             SoundManager &sound)
    : header("Journal"),
      bodyClear(0, BODY_Y, 240, 320 - BODY_Y, Theme::BG, true, Theme::BG),
      pageBadge(8, TAB_Y - 2, ButtonBadge::CONFIRM),
      statsTab(24, TAB_Y, "STATS", TAB_ON, 1),
      bestiaryTab(64, TAB_Y, "BESTIARY", TAB_OFF, 1),
      scrollHint(232, TAB_Y, "Up/Down scroll", TAB_OFF, 1, TR_DATUM),
      beasts(BEAST_ROWS),
      beastPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, Theme::FRAME, true, Theme::CARD, true, 4),
      beastStats(PANEL_TEXT_X, PANEL_Y + 6, "", Theme::TEXT, 1),
      beastFight(PANEL_TEXT_X, PANEL_Y + 18, "", Theme::TEXT_2, 1),
      beastDrops(PANEL_TEXT_X, PANEL_Y + 30),
      beastReward(PANEL_TEXT_X, PANEL_Y + 52, "", Theme::GOLD, 1),
      game(game),
      inventory(inventory),
      combat(combat),
      achievements(achievements),
      ascension(ascension),
      journal(journal),
      sound(sound),
      bestiary(false),
      scroll(0),
      entryCount(0)
{
    addElement(&header);
    setHeader(&header);
    addElement(&bodyClear);
    addElement(&pageBadge);
    addElement(&statsTab);
    addElement(&bestiaryTab);
    addElement(&scrollHint);

    for (uint8_t i = 0; i < LINES; i++)
    {
        lineLabel[i] = Text(LINE_X, LINE_Y + i * LINE_PITCH, "", LABEL_COLOR, 1);
        lineValue[i] = Text(LINE_RIGHT, LINE_Y + i * LINE_PITCH, "", Theme::TEXT, 1, TR_DATUM);
        addElement(&lineLabel[i]);
        addElement(&lineValue[i]);
    }

    for (uint8_t i = 0; i < BEAST_ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        addElement(&rows[i]);
    }
    addElement(&beastPanel);
    addElement(&beastStats, Theme::CARD);
    addElement(&beastFight, Theme::CARD);
    addElement(&beastDrops, Theme::CARD);
    addElement(&beastReward, Theme::CARD);

    beasts.setCount(enemyTotal());
    showPage(false, true);
}

// A page's elements decide their own visibility in its refresh, so this only hides them
void JournalScreen::showPage(bool visible, bool bestiaryPage)
{
    if (!bestiaryPage)
    {
        for (uint8_t i = 0; i < LINES; i++)
        {
            lineLabel[i].setVisible(visible);
            lineValue[i].setVisible(visible);
        }
        return;
    }
    for (uint8_t i = 0; i < BEAST_ROWS; i++)
    {
        rows[i].setVisible(visible);
    }
    beastPanel.setVisible(visible);
    beastStats.setVisible(visible);
    beastFight.setVisible(visible);
    beastDrops.setVisible(visible);
    beastReward.setVisible(visible);
}

void JournalScreen::onEnter(TFT_eSPI &tft)
{
    forceRefresh();
    Screen::onEnter(tft);
}

JournalScreen::Entry &JournalScreen::add(const char *label, bool heading)
{
    Entry &entry = entries[entryCount < MAX_ENTRIES - 1 ? entryCount++ : entryCount];
    entry.label = label;
    entry.value[0] = '\0';
    entry.heading = heading;
    return entry;
}

void JournalScreen::buildEntries()
{
    entryCount = 0;
    char buf[24];

    add("THIS RUN", true);
    formatDuration(journal.getRunSeconds(), add("Time played").value, sizeof(buf));
    snprintf(add("Gold earned").value, sizeof(buf), "%s", formatAmount(game.getTotalGoldEarned()).c_str());
    snprintf(add("Mines").value, sizeof(buf), "%s", formatAmount(game.getTotalClicks()).c_str());
    snprintf(add("Mining level").value, sizeof(buf), "%u", (unsigned)game.getMiningLevel());
    snprintf(add("Enemies defeated").value, sizeof(buf), "%s", formatAmount(combat.getTotalKills()).c_str());
    snprintf(add("Gold veins caught").value, sizeof(buf), "%lu", (unsigned long)game.getVeinsCaught());

    add("ALL RUNS", true);
    snprintf(add("Ascensions").value, sizeof(buf), "%u", (unsigned)ascension.getAscensions());
    formatDuration(journal.getTotalSeconds(), add("Time played").value, sizeof(buf));
    snprintf(add("Gold earned").value, sizeof(buf), "%s",
             formatAmount(ascension.getLifetimeGold(game.getTotalGoldEarned())).c_str());
    snprintf(add("Mines").value, sizeof(buf), "%s", formatAmount(journal.getLifetimeClicks(game)).c_str());
    snprintf(add("Best mining streak").value, sizeof(buf), "%u", (unsigned)journal.getBestStreak());
    snprintf(add("Enemies defeated").value, sizeof(buf), "%s", formatAmount(journal.getLifetimeKills(combat)).c_str());
    snprintf(add("Bosses defeated").value, sizeof(buf), "%s", formatAmount(journal.getBossKills()).c_str());
    snprintf(add("Gold veins caught").value, sizeof(buf), "%s", formatAmount(journal.getLifetimeVeins(game)).c_str());
    snprintf(add("Items found").value, sizeof(buf), "%u/%u", (unsigned)inventory.countOwnedItems(),
             (unsigned)ITEM_COUNT);
    snprintf(add("Awards").value, sizeof(buf), "%u/%u", (unsigned)achievements.getUnlockedCount(),
             (unsigned)ACHIEVEMENT_COUNT);

    if (journal.getRunCount() > 0)
    {
        add("LAST ASCENSIONS", true);
        static const char *const RUN_LABELS[Journal::HISTORY] = {"Last run", "2 runs ago", "3 runs ago",
                                                                 "4 runs ago", "5 runs ago"};
        for (uint8_t i = 0; i < journal.getRunCount(); i++)
        {
            const Journal::RunRecord &run = journal.getRun(i);
            char time[12];
            formatDuration(run.seconds, time, sizeof(time));
            snprintf(add(RUN_LABELS[i]).value, sizeof(buf), "+%lu pts in %s", (unsigned long)run.points, time);
        }
    }
}

void JournalScreen::refreshStats()
{
    buildEntries();
    uint8_t maxScroll = entryCount > LINES ? entryCount - LINES : 0;
    if (scroll > maxScroll)
    {
        scroll = maxScroll;
    }
    scrollHint.setText(maxScroll > 0 ? "Up/Down scroll" : "");

    for (uint8_t i = 0; i < LINES; i++)
    {
        uint8_t index = scroll + i;
        bool shown = index < entryCount;
        lineLabel[i].setVisible(shown);
        lineValue[i].setVisible(shown);
        if (!shown)
        {
            continue;
        }
        const Entry &entry = entries[index];
        lineLabel[i].setText(entry.label);
        lineLabel[i].setColor(entry.heading ? HEADING_COLOR : LABEL_COLOR);
        lineValue[i].setText(entry.value);
    }
}

bool JournalScreen::findEnemy(uint16_t index, uint8_t &zone, uint8_t &enemy) const
{
    for (uint8_t z = 0; z < ZONE_COUNT; z++)
    {
        if (index < ZONES[z].enemyCount)
        {
            zone = z;
            enemy = (uint8_t)index;
            return true;
        }
        index -= ZONES[z].enemyCount;
    }
    return false;
}

// An enemy is in the book once its zone has been open to the player
bool JournalScreen::isMet(uint8_t zone) const
{
    return combat.isZoneUnlocked(zone) || combat.getZoneKills(zone) > 0;
}

void JournalScreen::refreshBestiary()
{
    scrollHint.setText("Up/Down browse");
    Stats stats = resolvePlayerStats(game, inventory);
    char buf[40];

    uint8_t visible = beasts.getVisibleCount();
    for (uint8_t i = 0; i < BEAST_ROWS; i++)
    {
        ListRow &row = rows[i];
        uint8_t z, e;
        uint16_t index = beasts.getFirstVisible() + i;
        if (i >= visible || !findEnemy(index, z, e))
        {
            row.setVisible(false);
            continue;
        }
        row.setVisible(true);
        const EnemyDef &enemy = ZONES[z].enemies[e];
        bool met = isMet(z);
        row.setIconPixels(ZONES[z].icon, 16, 16);
        if (!met)
        {
            row.setTitle("???");
            snprintf(buf, sizeof(buf), "Somewhere in %s", ZONES[z].name);
            row.setSubtitle(buf);
            row.setValue("");
            row.setValueSub("");
            row.setState(index == beasts.getSelected() ? ListRow::SELECTED : ListRow::DIMMED);
            continue;
        }
        row.setTitle(enemy.name);
        if (enemy.boss)
        {
            snprintf(buf, sizeof(buf), "Boss of %s", ZONES[z].name);
            row.setSubtitle(buf);
        }
        else
        {
            row.setSubtitle(ZONES[z].name);
        }
        snprintf(buf, sizeof(buf), "DEF %s", formatCompact(enemy.defense).c_str());
        row.setValue(buf);
        // Green when the auto-attack gets through, gold when only the strike does
        bool idle = damageThrough(stats.attack, enemy.defense) > 0;
        bool strike = damageThrough(stats.attack * STRIKE_MULTIPLIER, enemy.defense) > 0;
        row.setValueColor(idle ? Theme::GOOD : strike ? Theme::GOLD : Theme::BAD);
        snprintf(buf, sizeof(buf), "HP %s", formatCompact(enemy.maxHp).c_str());
        row.setValueSub(buf);
        row.setState(index == beasts.getSelected() ? ListRow::SELECTED : ListRow::NORMAL);
    }

    uint8_t z, e;
    if (!findEnemy(beasts.getSelected(), z, e))
    {
        return;
    }
    const EnemyDef &enemy = ZONES[z].enemies[e];
    if (!isMet(z))
    {
        beastStats.setText("Not met yet");
        snprintf(buf, sizeof(buf), "Open %s to meet it", ZONES[z].name);
        beastFight.setText(buf);
        beastDrops.setCount(0);
        beastReward.setText("");
        return;
    }

    snprintf(buf, sizeof(buf), "ATK %s  DEF %s  HP %s  every %lu.%lus", formatCompact(enemy.attack).c_str(),
             formatCompact(enemy.defense).c_str(), formatCompact(enemy.maxHp).c_str(),
             enemy.attackIntervalMs / 1000, enemy.attackIntervalMs % 1000 / 100);
    beastStats.setText(buf);
    snprintf(buf, sizeof(buf), "You hit %s / %s, it hits %s",
             formatCompact(damageThrough(stats.attack, enemy.defense)).c_str(),
             formatCompact(damageThrough(stats.attack * STRIKE_MULTIPLIER, enemy.defense)).c_str(),
             formatCompact(damageThrough(enemy.attack, stats.defense)).c_str());
    beastFight.setText(buf);

    uint8_t shown = 0;
    uint8_t found = 0;
    for (uint8_t d = 0; d < enemy.dropCount && shown < IconStrip::MAX_ICONS; d++)
    {
        uint8_t itemId = enemy.drops[d].itemId;
        // A relic stays a secret until Relic Sense lets it drop
        if (!combat.canDrop(itemId))
        {
            continue;
        }
        bool owned = inventory.isOwned(itemId);
        beastDrops.setIcon(shown++, ITEMS[itemId].iconPixels, !owned);
        found += owned ? 1 : 0;
    }
    beastDrops.setCount(shown);
    snprintf(buf, sizeof(buf), "%u/%u drops found  %s gold/kill", (unsigned)found, (unsigned)shown,
             formatAmount(combat.getKillGold(enemy, z)).c_str());
    beastReward.setText(buf);
}

void JournalScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    statsTab.setColor(bestiary ? TAB_OFF : TAB_ON);
    bestiaryTab.setColor(bestiary ? TAB_ON : TAB_OFF);
    if (bestiary)
    {
        showPage(true, true);
        refreshBestiary();
    }
    else
    {
        refreshStats();
    }
}

// The page going away is forgotten rather than erased, since the clearing box under both
// pages repaints the whole body before the new page draws
void JournalScreen::onConfirmPress()
{
    sound.playMenu();
    if (bestiary)
    {
        for (uint8_t i = 0; i < BEAST_ROWS; i++)
        {
            rows[i].resetDrawn();
        }
        beastPanel.resetDrawn();
        beastStats.resetDrawn();
        beastFight.resetDrawn();
        beastDrops.resetDrawn();
        beastReward.resetDrawn();
    }
    else
    {
        for (uint8_t i = 0; i < LINES; i++)
        {
            lineLabel[i].resetDrawn();
            lineValue[i].resetDrawn();
        }
    }
    showPage(false, bestiary);
    bestiary = !bestiary;
    bodyClear.markDirty();
    forceRefresh();
}

void JournalScreen::onUpPress()
{
    if (bestiary)
    {
        beasts.previous();
    }
    else if (scroll > 0)
    {
        scroll--;
    }
    forceRefresh();
}

void JournalScreen::onDownPress()
{
    if (bestiary)
    {
        beasts.next();
    }
    else
    {
        scroll++; // Clamped by the next refresh
    }
    forceRefresh();
}
