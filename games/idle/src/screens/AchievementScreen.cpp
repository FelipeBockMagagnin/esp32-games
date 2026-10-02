#include "AchievementScreen.h"
#include "../game/GameConfig.h" // GOLD_COLOR

static const unsigned long REFRESH_MS = 200; // Nothing here moves fast

static const int16_t ROW_X = 5;
static const int16_t LIST_TOP = 32;
static const int16_t ROW_PITCH = 28;

static const int16_t FOOTER_Y = 292;

static const uint16_t UNLOCKED_COLOR = 0x07E0; // Green, same as a taken upgrade

AchievementScreen::AchievementScreen(Achievements &achievements, SoundManager &sound)
    : header("Awards", "Upgrade", "Inventory"),

      countText(10, FOOTER_Y, "0/0", 0xFFFF, 2),
      bonusText(230, FOOTER_Y, "+0% gold", GOLD_COLOR, 2, TR_DATUM),

      achievements(achievements),
      sound(sound),
      lastRefresh(0),
      lastUnlockedCount(0)
{
    addElement(&header);
    setHeader(&header);

    for (uint8_t i = 0; i < ListView::VISIBLE_ROWS; i++)
    {
        rows[i].setPosition(ROW_X, LIST_TOP + i * ROW_PITCH);
        addElement(&rows[i]);
    }

    addElement(&countText);
    addElement(&bonusText);

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

    char buf[20];
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
        const AchievementDef &def = ACHIEVEMENTS[id];
        bool unlocked = achievements.isUnlocked(id);

        row.setVisible(true);
        row.setTitle(def.name);
        row.setSubtitle(def.description);

        snprintf(buf, sizeof(buf), "+%u%%", (unsigned)def.bonusPercent);
        row.setValue(buf);
        row.setValueColor(unlocked ? UNLOCKED_COLOR : 0xFFFF);

        if (unlocked)
        {
            row.setIconBitmap(image_Ok_btn_bits, 9, 9, UNLOCKED_COLOR);
        }
        else
        {
            row.clearIcon();
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

    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)achievements.getUnlockedCount(),
             (unsigned)ACHIEVEMENT_COUNT);
    countText.setText(buf);

    snprintf(buf, sizeof(buf), "+%u%% gold", (unsigned)achievements.getProductionBonusPercent());
    bonusText.setText(buf);
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
