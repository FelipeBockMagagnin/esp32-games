#include "AchievementScreen.h"
#include "../game/GameConfig.h"
#include "../game/Format.h"


static const uint16_t CARD_BORDER = Theme::GOLD_FRAME;   // Dark amber, same frame as the gold cards
static const uint16_t PANEL_BORDER = Theme::FRAME;  // Dark slate
static const uint16_t UNLOCKED_COLOR = Theme::GOOD; // Green, same as a taken upgrade
static const uint16_t LOCKED_COLOR = Theme::TEXT_DIM;
static const uint16_t HINT_COLOR = Theme::TEXT_2;
static const uint16_t BAR_COLOR = Theme::GOLD_BAR;     // Amber fill, dark enough for the white label
static const uint16_t BAR_BG = Theme::GOLD_TRACK;
static const uint16_t BAR_BORDER = Theme::GOLD_BORDER;
static const uint16_t DONE_FILL = Theme::DONE_BAR;     // Dark green once complete

// Summary card
static const int16_t CARD_X = 5;
static const int16_t CARD_Y = 31;
static const int16_t CARD_W = 230;
static const int16_t CARD_H = 30;
static const int16_t CARD_RIGHT = CARD_X + CARD_W - 6;
static const int16_t COMPLETION_X = 104;

// List
static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 65;
static const int16_t ROW_PITCH = 28;

// Bonus card, two rows of four cells between the list and the panel
static const int16_t BONUS_Y = 234;
static const int16_t BONUS_H = 25;
static const int16_t BONUS_CELL_X = CARD_X + 7;
static const int16_t BONUS_CELL_PITCH = 56;
static const int16_t BONUS_ROW_Y = BONUS_Y + 4;
static const int16_t BONUS_ROW_PITCH = 10;
static const uint8_t BONUS_COLUMNS = 4;

// Progress panel; text (8px) and badges (11px) are centred on the row's middle
static const int16_t PANEL_X = 5;
static const int16_t PANEL_Y = 263;
static const int16_t PANEL_W = 230;
static const int16_t PANEL_H = 54;
static const int16_t TEXT_X = PANEL_X + 7;
static const int16_t TEXT_RIGHT = PANEL_X + PANEL_W - 7;
static const int16_t REWARD_Y = PANEL_Y + 6;
static const int16_t PROGRESS_Y = PANEL_Y + 18;
static const int16_t PROGRESS_H = 14;
static const int16_t HINT_ROW_Y = PANEL_Y + 39;
static const int16_t BADGE_OFFSET_Y = (8 - ButtonBadge::SIZE) / 2;
static const int16_t BROWSE_W = 6 * 6; // "Browse" at size 1

static const uint8_t NO_ACHIEVEMENT = 0xFF;

// The icon for an achievement follows what it measures, reusing the art of the screen
// where that counter grows: a building's own icon, the combat sword, the inventory armor
static const uint16_t *iconFor(const AchievementDef &def)
{
    switch (def.kind)
    {
    case AchKind::TOTAL_CLICKS:
        return image_upgrade_click_pixels;
    case AchKind::TOTAL_GOLD:
        return image_upgrade_production_pixels;
    case AchKind::TOTAL_BUILDINGS:
        return image_building_deep_shaft_pixels;
    case AchKind::BUILDING_LEVEL:
        return def.target < BUILDING_COUNT ? BUILDINGS[def.target].icon : image_building_deep_shaft_pixels;
    case AchKind::UPGRADES_BOUGHT:
        return image_award_scroll_pixels;
    case AchKind::MINING_LEVEL:
        return image_building_pickaxe_pixels;
    case AchKind::TOTAL_KILLS:
        return image_sword_02b_pixels;
    case AchKind::ZONE_KILLS:
        return image_award_skull_pixels;
    case AchKind::ITEMS_OWNED:
        return image_armor_01b_pixels;
    case AchKind::ITEM_LEVEL:
        return image_building_rune_forge_pixels;
    case AchKind::ACHIEVEMENTS:
        return image_award_trophy_pixels;
    case AchKind::VEINS_CAUGHT:
        return image_icon_gold_vein_pixels;
    case AchKind::GEAR_SET_MAXED:
        // The set's body armour stands for it
        for (uint8_t i = 0; i < ITEM_COUNT; i++)
        {
            if (ITEMS[i].slot == EquipSlot::BODY && ITEMS[i].tier == def.target)
            {
                return ITEMS[i].iconPixels;
            }
        }
        return image_armor_01b_pixels;
    case AchKind::ITEM_MAXED:
        return def.target < ITEM_COUNT ? ITEMS[def.target].iconPixels : image_award_trophy_pixels;
    }
    return image_award_trophy_pixels;
}

static const char *const REWARD_NAMES[ACH_REWARD_COUNT] = {
    "gold/s", "ATK and DEF", "respawn time", "revive time", "guard time", "spell cooldown", "kill gold", "drop chance"};

// Indexed by reward minus one, production having the summary card. Three letters, like
// the inventory's stat codes, so four cells fit across.
static const char *const BONUS_CODES[ACH_REWARD_COUNT - 1] = {"A/D", "RSP", "REV", "GRD", "SPL", "KIL", "DRP"};

// Each reward in the colour of what it touches elsewhere: gold for the economy, the
// combat screen's stat colours for the fight, the spell purple, the guard's cyan
static const uint16_t REWARD_COLORS[ACH_REWARD_COUNT] = {
    Theme::GOLD,       // PRODUCTION
    Theme::ATTACK,     // ATTACK_DEFENSE
    Theme::TEXT,       // RESPAWN_CUT
    Theme::HP,         // REVIVE_CUT
    Theme::INFO,       // GUARD_TIME
    Theme::EPIC,       // SPELL_COOLDOWN_CUT
    Theme::ENEMY_NAME, // KILL_GOLD
    Theme::DEFENSE,    // DROP_CHANCE, the rare-item blue
};

static uint16_t rewardColor(AchReward reward)
{
    return REWARD_COLORS[(uint8_t)reward];
}

static bool isCut(AchReward reward)
{
    return reward == AchReward::RESPAWN_CUT || reward == AchReward::REVIVE_CUT ||
           reward == AchReward::SPELL_COOLDOWN_CUT;
}

AchievementScreen::AchievementScreen(Achievements &achievements, GameState &game, Inventory &inventory,
                                     CombatState &combat, SoundManager &sound)
    : header("Awards"),

      summaryCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, true, Theme::CARD, true, 4),
      trophyIcon(CARD_X + 6, CARD_Y + 7, 16, 16, image_award_trophy_pixels, true, 0x0000),
      countText(CARD_X + 28, CARD_Y + 8, "0/0", Theme::TEXT, 2),
      bonusText(CARD_RIGHT, CARD_Y + 5, "+0% gold/s", Theme::GOLD, 1, TR_DATUM),
      completionBar(COMPLETION_X, CARD_Y + 17, CARD_RIGHT - COMPLETION_X, 8, 0, 1, BAR_COLOR, BAR_BG, BAR_BORDER),

      list(ROWS),

      bonusCard(CARD_X, BONUS_Y, CARD_W, BONUS_H, PANEL_BORDER, true, Theme::CARD, true, 4),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, true, Theme::CARD, true, 4),
      rewardText(TEXT_X, REWARD_Y, "", Theme::GOLD, 1),
      statusText(TEXT_RIGHT, REWARD_Y, "", LOCKED_COLOR, 1, TR_DATUM),
      progressBar(TEXT_X, PROGRESS_Y, TEXT_RIGHT - TEXT_X, PROGRESS_H, 0, 100, BAR_COLOR, BAR_BG, BAR_BORDER),
      nextBadge(TEXT_X, HINT_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::CONFIRM),
      nextLabel(TEXT_X + ButtonBadge::SIZE + 4, HINT_ROW_Y, "Next locked", HINT_COLOR, 1),
      upBadge(TEXT_RIGHT - BROWSE_W - 2 * ButtonBadge::SIZE - 6, HINT_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::UP),
      downBadge(TEXT_RIGHT - BROWSE_W - ButtonBadge::SIZE - 4, HINT_ROW_Y + BADGE_OFFSET_Y, ButtonBadge::DOWN),
      browseLabel(TEXT_RIGHT, HINT_ROW_Y, "Browse", HINT_COLOR, 1, TR_DATUM),

      achievements(achievements),
      game(game),
      inventory(inventory),
      combat(combat),
      sound(sound),
      lastUnlockedCount(0)
{
    refreshMs = 200; // Nothing here moves fast
    addElement(&header);
    setHeader(&header);

    addElement(&summaryCard);
    addElement(&trophyIcon, Theme::CARD);
    addElement(&countText, Theme::CARD);
    addElement(&bonusText, Theme::CARD);
    addElement(&completionBar, Theme::CARD);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        rowAchievement[i] = NO_ACHIEVEMENT;
        addElement(&rows[i]);
    }

    addElement(&bonusCard);
    for (uint8_t i = 0; i < BONUS_CELLS; i++)
    {
        bonusCells[i].setPosition(BONUS_CELL_X + (i % BONUS_COLUMNS) * BONUS_CELL_PITCH,
                                  BONUS_ROW_Y + (i / BONUS_COLUMNS) * BONUS_ROW_PITCH);
        addElement(&bonusCells[i], Theme::CARD);
    }

    addElement(&detailPanel);
    addElement(&rewardText, Theme::CARD);
    addElement(&statusText, Theme::CARD);
    addElement(&progressBar, Theme::CARD);
    addElement(&nextBadge, Theme::CARD);
    addElement(&nextLabel, Theme::CARD);
    addElement(&upBadge, Theme::CARD);
    addElement(&downBadge, Theme::CARD);
    addElement(&browseLabel, Theme::CARD);

    list.setCount(ACHIEVEMENT_COUNT);
}

void AchievementScreen::update(unsigned long now)
{
    // An achievement can land while this screen is open
    if (achievements.getUnlockedCount() != lastUnlockedCount)
    {
        lastUnlockedCount = achievements.getUnlockedCount();
        forceRefresh();
    }

    if (!refreshDue(now))
    {
        return;
    }

    char buf[24];
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
        const AchievementDef &def = ACHIEVEMENTS[id];
        bool unlocked = achievements.isUnlocked(id);
        row.setVisible(true);

        if (rowAchievement[i] != id)
        {
            rowAchievement[i] = id;
            row.setTitle(def.name);
            row.setSubtitle(def.description);
            row.setIconPixels(iconFor(def), 16, 16);
            snprintf(buf, sizeof(buf), "%c%u%%", isCut(def.reward) ? '-' : '+', (unsigned)def.bonusPercent);
            row.setValue(buf);
        }
        row.setValueColor(rewardColor(def.reward));

        // A locked one shows how far along it is, so the list doubles as a to-do list. Only
        // a percentage fits beside the subtitle; the panel below has the full count. Capped
        // at 99 so it never claims 100% before evaluate() has actually unlocked it.
        if (unlocked)
        {
            row.setValueSub("done");
        }
        else
        {
            uint64_t progress = achievements.getProgress(def, game, inventory, combat);
            uint32_t percent = def.amount == 0 ? 0 : (uint32_t)(progress * 100 / def.amount);
            snprintf(buf, sizeof(buf), "%lu%%", (unsigned long)(percent > 99 ? 99 : percent));
            row.setValueSub(buf);
        }

        if (id == list.getSelected())
        {
            row.setState(ListRow::SELECTED);
        }
        else
        {
            row.setState(unlocked ? ListRow::OWNED : ListRow::DIMMED);
        }
    }

    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)achievements.getUnlockedCount(), (unsigned)ACHIEVEMENT_COUNT);
    countText.setText(buf);
    completionBar.setProgress(achievements.getUnlockedCount(), ACHIEVEMENT_COUNT);

    snprintf(buf, sizeof(buf), "+%u%% gold/s", (unsigned)achievements.getProductionBonusPercent());
    bonusText.setText(buf);

    for (uint8_t i = 0; i < BONUS_CELLS; i++)
    {
        AchReward reward = (AchReward)(i + 1);
        uint32_t percent = achievements.getRewardPercent(reward);
        snprintf(buf, sizeof(buf), "%s %c%lu%%", BONUS_CODES[i], isCut(reward) ? '-' : '+', (unsigned long)percent);
        bonusCells[i].setText(buf);
        bonusCells[i].setColor(percent > 0 ? rewardColor(reward) : LOCKED_COLOR);
    }

    updateDetail();
}

void AchievementScreen::updateDetail()
{
    uint8_t id = (uint8_t)list.getSelected();
    const AchievementDef &def = ACHIEVEMENTS[id];
    bool unlocked = achievements.isUnlocked(id);
    char buf[40];

    snprintf(buf, sizeof(buf), "Reward: %c%u%% %s", isCut(def.reward) ? '-' : '+', (unsigned)def.bonusPercent,
             REWARD_NAMES[(uint8_t)def.reward]);
    rewardText.setText(buf);
    rewardText.setColor(rewardColor(def.reward));
    statusText.setText(unlocked ? "UNLOCKED" : "LOCKED");
    statusText.setColor(unlocked ? UNLOCKED_COLOR : LOCKED_COLOR);

    if (unlocked)
    {
        progressBar.setProgress(100, 100);
        progressBar.setFillColor(DONE_FILL);
        progressBar.setLabel("Complete");
        return;
    }

    uint64_t progress = achievements.getProgress(def, game, inventory, combat);
    // Progress can only pass the amount between two evaluate() passes; cap it meanwhile
    uint32_t percent = (def.amount == 0 || progress >= def.amount) ? 100 : (uint32_t)(progress * 100 / def.amount);
    progressBar.setProgress(percent, 100);
    progressBar.setFillColor(BAR_COLOR);
    snprintf(buf, sizeof(buf), "%s / %s  (%lu%%)", formatAmount(progress).c_str(), formatAmount(def.amount).c_str(),
             (unsigned long)percent);
    progressBar.setLabel(buf);
}

void AchievementScreen::onUpPress()
{
    list.previous();
    forceRefresh();
}

void AchievementScreen::onDownPress()
{
    list.next();
    forceRefresh();
}

void AchievementScreen::onConfirmPress()
{
    // Jump to the next locked achievement after the current one, wrapping around
    uint16_t start = list.getSelected();
    for (uint8_t step = 1; step <= ACHIEVEMENT_COUNT; step++)
    {
        uint16_t candidate = (start + step) % ACHIEVEMENT_COUNT;
        if (!achievements.isUnlocked((uint8_t)candidate))
        {
            list.setSelected(candidate);
            sound.playMenu();
            forceRefresh();
            return;
        }
    }

    // Everything is unlocked, so there is nowhere to jump
    sound.playError();
}
