#include "AscendScreen.h"
#include "../game/Format.h"
#include "../game/Achievements.h"
#include "../game/CombatConfig.h"

static const uint16_t ACCENT = Theme::EPIC;
static const uint16_t PANEL_BORDER = Theme::FRAME;
static const uint16_t HINT_COLOR = Theme::TEXT_2;
static const uint16_t LOCKED_COLOR = Theme::TEXT_DIM;
static const uint16_t AFFORD_COLOR = Theme::GOOD;
static const uint16_t BAR_COLOR = Theme::GOLD_BAR;
static const uint16_t BAR_BG = Theme::GOLD_TRACK;
static const uint16_t BAR_BORDER = Theme::GOLD_BORDER;

// Summary card
static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 31;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 46;
static const int16_t CARD_RIGHT = CARD_X + CARD_W - 6;
static const int16_t BAR_Y = CARD_Y + 27;
static const int16_t BAR_H = 14;

// List; each level of the tree indents a talent's row by this much
static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 81;
static const int16_t ROW_PITCH = 28;
static const int16_t INDENT = 10;

// Detail panel; text (8px) and the badge (11px) share the bottom row's middle
static const int16_t PANEL_X = 5;
static const int16_t PANEL_Y = 251;
static const int16_t PANEL_W = 230;
static const int16_t PANEL_H = 66;
static const int16_t TEXT_X = PANEL_X + 7;
static const int16_t TEXT_RIGHT = PANEL_X + PANEL_W - 7;
static const int16_t STATUS_Y = PANEL_Y + 7;
static const int16_t EFFECT_Y = PANEL_Y + 25;
static const int16_t ACTION_Y = PANEL_Y + 48;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2;

struct EffectStyle
{
    const char *label;
    uint16_t color;
    const char *unit; // After the value: "%" for the percentages
};

// Indexed by TalentEffect, in the colours the same bonus wears elsewhere
static const EffectStyle EFFECT_STYLES[TALENT_EFFECT_COUNT] = {
    {"Gold/s", Theme::GOLD, "%"},          // PRODUCTION
    {"Per click", Theme::GOLD, "%"},       // CLICK
    {"Buildings", Theme::GOLD, "%"},       // BUILDING_DISCOUNT
    {"Gold/s", Theme::GOLD, "%"},          // PRODUCTION_PER_AWARD
    {"ATK and DEF", Theme::ATTACK, "%"},   // ATTACK_DEFENSE
    {"Max HP", Theme::HP, "%"},            // MAX_HP
    {"Drop chance", Theme::DEFENSE, "%"},  // DROP_CHANCE
    {"Kill gold", Theme::ENEMY_NAME, "%"}, // KILL_GOLD
    {"Vein rate", Theme::GOLD, "%"},       // VEIN_FREQUENCY
    {"Vein power", Theme::GOLD, "%"},      // VEIN_POWER
    {"Combo cap", Theme::GOLD, "%"},       // CLICK_COMBO
    {"XP per click", Theme::GOLD, ""},     // MINING_XP
    {"", Theme::GOLD, ""},                 // HEAD_START, worded in formatEffect
    {"Per level before", Theme::GOLD, "%"}, // SYNERGY
    {"", Theme::HP, ""},                   // SECOND_WIND, worded in formatEffect
    {"Twin chance", Theme::DEFENSE, "%"},  // TWIN_DROP
    {"Relics", Theme::GOLD, ""},           // RELIC_HUNT
    {"Maps kept", Theme::EPIC, ""},        // KEEP_MAPS
    {"", Theme::EPIC, ""},                 // DEEP_ZONES, worded in formatEffect
    {"Vein time", Theme::GOLD, "s"},       // VEIN_LINGER
};

AscendScreen::AscendScreen(const GameState &game, Ascension &ascension, SoundManager &sound,
                           ScreenManager &screenManager, Screen &confirmScreen)
    : header("Ascend"),

      summaryCard(CARD_X, CARD_Y, CARD_W, CARD_H, ACCENT, true, Theme::CARD, true, 4),
      pointsText(CARD_X + 7, CARD_Y + 6, "0 pts", ACCENT, 2),
      ascensionsText(CARD_RIGHT, CARD_Y + 5, "", HINT_COLOR, 1, TR_DATUM),
      bonusText(CARD_RIGHT, CARD_Y + 15, "", Theme::GOLD, 1, TR_DATUM),
      nextPointBar(CARD_X + 7, BAR_Y, CARD_W - 14, BAR_H, 0, 100, BAR_COLOR, BAR_BG, BAR_BORDER),

      list(ROWS),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, true, Theme::CARD, true, 4),
      statusText(TEXT_X, STATUS_Y, "", Theme::TEXT, 1),
      levelText(TEXT_RIGHT, STATUS_Y, "", HINT_COLOR, 1, TR_DATUM),
      effectText(TEXT_X, EFFECT_Y, "", Theme::TEXT, 1),
      actionBadge(TEXT_X, ACTION_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      actionLabel(TEXT_X + ButtonBadge::SIZE + 4, ACTION_Y, "", Theme::TEXT, 1),
      priceText(TEXT_RIGHT, ACTION_Y, "", HINT_COLOR, 1, TR_DATUM),

      game(game),
      ascension(ascension),
      sound(sound),
      screenManager(screenManager),
      confirmScreen(confirmScreen)
{
    refreshMs = 200;
    addElement(&header);
    setHeader(&header);

    addElement(&summaryCard);
    addElement(&pointsText, Theme::CARD);
    addElement(&ascensionsText, Theme::CARD);
    addElement(&bonusText, Theme::CARD);
    addElement(&nextPointBar, Theme::CARD);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        rowItem[i] = NO_ITEM;
        addElement(&rows[i]);
    }

    addElement(&detailPanel);
    addElement(&statusText, Theme::CARD);
    addElement(&levelText, Theme::CARD);
    addElement(&effectText, Theme::CARD);
    addElement(&actionBadge, Theme::CARD);
    addElement(&actionLabel, Theme::CARD);
    addElement(&priceText, Theme::CARD);

    list.setCount(LIST_COUNT);
}

void AscendScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }

    uint64_t runGold = game.getTotalGoldEarned();
    char buf[40];

    snprintf(buf, sizeof(buf), "%lu pts", (unsigned long)ascension.getPointsAvailable());
    pointsText.setText(buf);
    snprintf(buf, sizeof(buf), "Ascended %ux", (unsigned)ascension.getAscensions());
    ascensionsText.setText(buf);
    snprintf(buf, sizeof(buf), "+%lu%% gold/s", (unsigned long)game.getAscensionProductionPercent());
    bonusText.setText(buf);

    // Progress of this run's gold toward the next point it would bank
    uint64_t target = ascension.getRunGoldForNextPoint(runGold);
    uint32_t percent = target == 0 ? 100 : (uint32_t)(runGold >= target ? 100 : runGold * 100 / target);
    nextPointBar.setProgress(percent, 100);
    snprintf(buf, sizeof(buf), "Next pt %s / %s", formatAmount(runGold).c_str(), formatAmount(target).c_str());
    nextPointBar.setLabel(buf);

    uint8_t visible = list.getVisibleCount();
    for (uint8_t i = 0; i < ROWS; i++)
    {
        if (i >= visible)
        {
            rows[i].setVisible(false);
            continue;
        }
        uint8_t item = (uint8_t)(list.getFirstVisible() + i);
        rows[i].setVisible(true);
        refreshRow(rows[i], item, item == list.getSelected(), i);
    }

    refreshDetail();
}

void AscendScreen::refreshRow(ListRow &row, uint8_t item, bool selected, uint8_t slot)
{
    char buf[24];

    if (item == 0)
    {
        uint32_t pending = ascension.getPendingPoints(game.getTotalGoldEarned());
        if (rowItem[slot] != item)
        {
            rowItem[slot] = item;
            row.setPosition(ROW_X, row.y);
            row.setSize(ListRow::DEFAULT_W, ListRow::DEFAULT_H);
            row.setTitle("Ascend");
            row.setIconPixels(image_item_core_sigil_pixels, 16, 16);
            row.setValueSub("points");
        }
        row.setSubtitle(pending > 0 ? "Restart the run for points" : "Earn more gold first");
        snprintf(buf, sizeof(buf), "+%lu", (unsigned long)pending);
        row.setValue(buf);
        row.setValueColor(pending > 0 ? ACCENT : LOCKED_COLOR);
        row.setTitleColor(ACCENT);
        row.setFrameColor(ACCENT);
        row.setState(selected ? ListRow::SELECTED : pending > 0 ? ListRow::NORMAL : ListRow::DIMMED);
        return;
    }

    uint8_t id = item - 1;
    const TalentDef &def = TALENTS[id];
    if (rowItem[slot] != item)
    {
        rowItem[slot] = item;
        int16_t indent = ascension.getTalentDepth(id) * INDENT;
        row.setPosition(ROW_X + indent, row.y);
        row.setSize(ListRow::DEFAULT_W - indent, ListRow::DEFAULT_H);
        row.setTitle(def.name);
        row.setSubtitle(def.description);
        row.setIconPixels(def.icon, 16, 16);
        row.setFrameColor(ListRow::DEFAULT_FRAME_COLOR);
    }

    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)ascension.getTalentLevel(id), (unsigned)def.maxLevel);
    row.setValue(buf);
    bool maxed = ascension.isTalentMaxed(id);
    if (maxed)
    {
        row.setValueSub("max");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lu pts", (unsigned long)ascension.getTalentCost(id));
        row.setValueSub(buf);
    }
    row.setValueColor(ascension.canBuyTalent(id) ? AFFORD_COLOR : Theme::TEXT);
    // The ascend row may have used this row and left its title colour behind
    row.setTitleColor(maxed ? AFFORD_COLOR : Theme::TEXT);

    if (selected)
    {
        row.setState(ListRow::SELECTED);
    }
    else if (!ascension.isTalentOpen(id))
    {
        row.setState(ListRow::DIMMED);
    }
    else
    {
        row.setState(maxed ? ListRow::OWNED : ListRow::NORMAL);
    }
}

// The talent's own share of its bonus, now and with one more level
void AscendScreen::formatEffect(uint8_t id, char *buf, size_t size) const
{
    const TalentDef &def = TALENTS[id];
    uint32_t level = ascension.getTalentLevel(id);
    uint32_t scale = def.effect == TalentEffect::PRODUCTION_PER_AWARD ? game.getAchievements().getUnlockedCount() : 1;
    uint32_t now = def.perLevel * level * scale;
    char sign = def.effect == TalentEffect::BUILDING_DISCOUNT ? '-' : '+';
    const EffectStyle &style = EFFECT_STYLES[(uint8_t)def.effect];
    bool maxed = ascension.isTalentMaxed(id);
    uint32_t next = level + 1;

    switch (def.effect)
    {
    case TalentEffect::HEAD_START:
        snprintf(buf, size, maxed ? "Start: %lu %ss" : "Start: %lu %ss > %lu",
                 (unsigned long)(HEAD_START_LEVELS[0] * level), BUILDINGS[0].name,
                 (unsigned long)(HEAD_START_LEVELS[0] * next));
        return;
    case TalentEffect::SECOND_WIND:
    {
        unsigned long nowMin = (SECOND_WIND_BASE_MS / 60000) - def.perLevel * level;
        unsigned long nextMin = (SECOND_WIND_BASE_MS / 60000) - def.perLevel * next;
        if (level == 0)
        {
            snprintf(buf, size, "Revive every %lu min", nextMin);
        }
        else
        {
            snprintf(buf, size, maxed ? "Revive every %lu min" : "Revive every %lu > %lu min", nowMin, nextMin);
        }
        return;
    }
    case TalentEffect::DEEP_ZONES:
    {
        // The zones it opens are the last rows of ZONES, in order
        uint8_t first = ZONE_COUNT - def.maxLevel;
        snprintf(buf, size, maxed ? "Opened %s" : "Opens %s", ZONES[first + (maxed ? level - 1 : level)].name);
        return;
    }
    default:
        break;
    }

    if (maxed)
    {
        snprintf(buf, size, "%s %c%lu%s", style.label, sign, (unsigned long)now, style.unit);
        return;
    }
    snprintf(buf, size, "%s %c%lu%s > %c%lu%s", style.label, sign, (unsigned long)now, style.unit, sign,
             (unsigned long)(now + def.perLevel * scale), style.unit);
}

void AscendScreen::refreshAscendDetail()
{
    uint64_t runGold = game.getTotalGoldEarned();
    uint32_t pending = ascension.getPendingPoints(runGold);
    char buf[40];

    snprintf(buf, sizeof(buf), "Run gold %s", formatAmount(runGold).c_str());
    statusText.setText(buf);
    statusText.setColor(Theme::GOLD);
    snprintf(buf, sizeof(buf), "Lifetime %s", formatAmount(ascension.getLifetimeGold(runGold)).c_str());
    levelText.setText(buf);

    // What the new points add by themselves, before any is spent
    uint32_t bonus = game.getAscensionProductionPercent();
    snprintf(buf, sizeof(buf), "Gold/s +%lu%% > +%lu%%", (unsigned long)bonus,
             (unsigned long)(bonus + pending * ASCEND_PRODUCTION_PER_POINT));
    effectText.setText(buf);
    effectText.setColor(Theme::GOLD);

    actionBadge.setDimmed(pending == 0);
    actionLabel.setText("Ascend");
    snprintf(buf, sizeof(buf), "+%lu pts", (unsigned long)pending);
    priceText.setText(buf);
    priceText.setColor(pending > 0 ? ACCENT : LOCKED_COLOR);
}

void AscendScreen::refreshDetail()
{
    uint8_t item = (uint8_t)list.getSelected();
    if (item == 0)
    {
        refreshAscendDetail();
        return;
    }

    uint8_t id = item - 1;
    const TalentDef &def = TALENTS[id];
    char buf[40];

    bool open = ascension.isTalentOpen(id);
    bool maxed = ascension.isTalentMaxed(id);
    if (!open)
    {
        snprintf(buf, sizeof(buf), "Needs %s", TALENTS[def.parent].name);
        statusText.setText(buf);
        statusText.setColor(LOCKED_COLOR);
    }
    else
    {
        statusText.setText(maxed ? "MAXED" : "OPEN");
        statusText.setColor(maxed ? AFFORD_COLOR : Theme::TEXT);
    }
    snprintf(buf, sizeof(buf), "Level %u/%u", (unsigned)ascension.getTalentLevel(id), (unsigned)def.maxLevel);
    levelText.setText(buf);

    formatEffect(id, buf, sizeof(buf));
    effectText.setText(buf);
    effectText.setColor(EFFECT_STYLES[(uint8_t)def.effect].color);

    bool affordable = ascension.canBuyTalent(id);
    actionBadge.setDimmed(!affordable);
    actionLabel.setText("Buy level");
    if (maxed)
    {
        priceText.setText("max");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lu pts", (unsigned long)ascension.getTalentCost(id));
        priceText.setText(buf);
    }
    priceText.setColor(affordable ? AFFORD_COLOR : HINT_COLOR);
}

void AscendScreen::onUpPress()
{
    list.previous();
    forceRefresh();
}

void AscendScreen::onDownPress()
{
    list.next();
    forceRefresh();
}

void AscendScreen::onConfirmPress()
{
    uint8_t item = (uint8_t)list.getSelected();
    if (item == 0)
    {
        if (ascension.getPendingPoints(game.getTotalGoldEarned()) > 0)
        {
            sound.playMenu();
            screenManager.openModal(&confirmScreen);
        }
        else
        {
            sound.playError();
        }
        return;
    }

    if (ascension.buyTalent(item - 1))
    {
        sound.playBuy();
    }
    else
    {
        sound.playError();
    }
    forceRefresh();
}
