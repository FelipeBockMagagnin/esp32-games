#include "AchievementScreen.h"
#include "../game/GameConfig.h" // GOLD_COLOR
#include "../game/Format.h"

static const unsigned long REFRESH_MS = 200; // Nothing here moves fast

static const uint16_t CARD_BORDER = 0x7AE2;   // Dark amber, same frame as the gold cards
static const uint16_t PANEL_BORDER = 0x31A6;  // Dark slate
static const uint16_t SELECTED_FILL = 0x2124; // Faint highlight behind the selected row
static const uint16_t UNLOCKED_COLOR = 0x07E0; // Green, same as a taken upgrade
static const uint16_t LOCKED_COLOR = 0x7BEF;
static const uint16_t HINT_COLOR = 0x94B2;
static const uint16_t BAR_COLOR = 0x9B00;     // Amber fill, dark enough for the white label
static const uint16_t BAR_BG = 0x2080;
static const uint16_t BAR_BORDER = 0x7AE2;
static const uint16_t DONE_FILL = 0x0400;     // Dark green once complete

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
    }
    return image_award_trophy_pixels;
}

AchievementScreen::AchievementScreen(Achievements &achievements, GameState &game, Inventory &inventory,
                                     CombatState &combat, SoundManager &sound)
    : header("Awards", "Upgrade", "Inventory"),

      summaryCard(CARD_X, CARD_Y, CARD_W, CARD_H, CARD_BORDER, false, 0x0000, true, 4),
      trophyIcon(CARD_X + 6, CARD_Y + 7, 16, 16, image_award_trophy_pixels, true, 0x0000),
      countText(CARD_X + 28, CARD_Y + 8, "0/0", 0xFFFF, 2),
      bonusText(CARD_RIGHT, CARD_Y + 5, "+0% gold/s", GOLD_COLOR, 1, TR_DATUM),
      completionBar(COMPLETION_X, CARD_Y + 17, CARD_RIGHT - COMPLETION_X, 8, 0, 1, BAR_COLOR, BAR_BG, BAR_BORDER),

      list(ROWS),

      detailPanel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, PANEL_BORDER, false, 0x0000, true, 4),
      rewardText(TEXT_X, REWARD_Y, "", GOLD_COLOR, 1),
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
      lastRefresh(0),
      lastUnlockedCount(0)
{
    addElement(&header);
    setHeader(&header);

    addElement(&summaryCard);
    addElement(&trophyIcon);
    addElement(&countText);
    addElement(&bonusText);
    addElement(&completionBar);

    for (uint8_t i = 0; i < ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        rows[i].setSelectedFill(SELECTED_FILL);
        rowAchievement[i] = NO_ACHIEVEMENT;
        addElement(&rows[i]);
    }

    addElement(&detailPanel);
    addElement(&rewardText);
    addElement(&statusText);
    addElement(&progressBar);
    addElement(&nextBadge);
    addElement(&nextLabel);
    addElement(&upBadge);
    addElement(&downBadge);
    addElement(&browseLabel);

    list.setCount(ACHIEVEMENT_COUNT);
}

void AchievementScreen::update(unsigned long now)
{
    // An achievement can land while this screen is open
    if (achievements.getUnlockedCount() != lastUnlockedCount)
    {
        lastUnlockedCount = achievements.getUnlockedCount();
        lastRefresh = 0;
    }

    if (lastRefresh != 0 && now - lastRefresh < REFRESH_MS)
    {
        return;
    }
    lastRefresh = now;

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
            snprintf(buf, sizeof(buf), "+%u%%", (unsigned)def.bonusPercent);
            row.setValue(buf);
        }
        row.setValueColor(unlocked ? UNLOCKED_COLOR : GOLD_COLOR);

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

    updateDetail();
}

void AchievementScreen::updateDetail()
{
    uint8_t id = (uint8_t)list.getSelected();
    const AchievementDef &def = ACHIEVEMENTS[id];
    bool unlocked = achievements.isUnlocked(id);
    char buf[40];

    snprintf(buf, sizeof(buf), "Reward: +%u%% gold/s", (unsigned)def.bonusPercent);
    rewardText.setText(buf);
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
    lastRefresh = 0;
}

void AchievementScreen::onDownPress()
{
    list.next();
    lastRefresh = 0;
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
            lastRefresh = 0;
            return;
        }
    }

    // Everything is unlocked, so there is nowhere to jump
    sound.playError();
}
