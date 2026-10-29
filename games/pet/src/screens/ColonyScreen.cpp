#include "ColonyScreen.h"
#include "../game/Format.h"

static const int16_t SUMMARY_Y = 34;
static const int16_t FIRST_ROW_Y = 48;
static const int16_t ROW_PITCH = ListRow::DEFAULT_H + 3;

ColonyScreen::ColonyScreen(const World &world, Colony &colony, SoundManager &sound)
    : header("Colony"),
      summaryText(5, SUMMARY_Y, "", Theme::TEXT),
      incomeText(235, SUMMARY_Y, "", Theme::GOLD, 1, TR_DATUM),
      emptyText(120, 140, "Everyone left. Adopt in the Shop.", Theme::TEXT_DIM, 1, TC_DATUM),
      list(ROW_COUNT),
      world(world),
      colony(colony),
      sound(sound),
      armed(false),
      armedRevision(0)
{
    addElement(&header);
    setHeader(&header);
    addElement(&summaryText);
    addElement(&incomeText);
    addElement(&emptyText);
    for (uint8_t i = 0; i < ROW_COUNT; i++)
    {
        rows[i].setPosition(5, FIRST_ROW_Y + i * ROW_PITCH);
        addElement(&rows[i]);
    }
}

void ColonyScreen::onEnter(TFT_eSPI &tft)
{
    list.reset();
    armed = false;
    refresh();
    Screen::onEnter(tft);
}

void ColonyScreen::update(unsigned long now)
{
    if (!refreshDue(now))
    {
        return;
    }
    refresh();
}

void ColonyScreen::refresh()
{
    char buf[40];
    snprintf(buf, sizeof(buf), "%u critters, %u beds", (unsigned)colony.getCount(), (unsigned)colony.getCapacity());
    summaryText.setText(buf);
    summaryText.setColor(colony.isHoused() ? Theme::TEXT : Theme::BAD);
    snprintf(buf, sizeof(buf), "+%s/min", formatAmount(colony.getTotalCoinsPerMinute()).c_str());
    incomeText.setText(buf);
    emptyText.setVisible(colony.getCount() == 0);
    // Someone joined or left: the selection may be another critter now
    if (armed && colony.getRevision() != armedRevision)
    {
        armed = false;
    }

    list.setCount(colony.getCount());
    for (uint8_t r = 0; r < ROW_COUNT; r++)
    {
        ListRow &row = rows[r];
        uint16_t i = list.getFirstVisible() + r;
        row.setVisible(r < list.getVisibleCount());
        if (r >= list.getVisibleCount())
        {
            continue;
        }
        const Critter &c = colony.get(i);
        bool releasing = armed && i == list.getSelected();
        if (releasing)
        {
            snprintf(buf, sizeof(buf), "Release %s?", CRITTER_NAMES[c.name]);
            row.setTitle(buf);
            row.setTitleColor(Theme::BAD);
            row.setSubtitle("Confirm again to let it go");
            row.setValue("");
            row.setValueSub("");
            row.setState(ListRow::SELECTED);
            continue;
        }
        row.setTitle(CRITTER_NAMES[c.name]);
        row.setTitleColor(Theme::TEXT);
        snprintf(buf, sizeof(buf), "%s, %s", SPECIES[c.species].name, describeCritter(c));
        row.setSubtitle(buf);
        uint8_t mood = colony.getMood(i);
        snprintf(buf, sizeof(buf), "Mood %u", (unsigned)mood);
        row.setValue(buf);
        row.setValueColor(mood >= BREED_MOOD ? Theme::GOOD : (mood < MOOD_BASE ? Theme::BAD : Theme::TEXT));
        snprintf(buf, sizeof(buf), "+%s/min", formatAmount(colony.getCoinsPerMinute(i)).c_str());
        row.setValueSub(buf);
        row.setState(i == list.getSelected() ? ListRow::SELECTED : ListRow::NORMAL);
    }
}

void ColonyScreen::onUpPress()
{
    armed = false;
    list.previous();
    sound.playMenu();
    forceRefresh();
}

void ColonyScreen::onDownPress()
{
    armed = false;
    list.next();
    sound.playMenu();
    forceRefresh();
}

void ColonyScreen::onConfirmPress()
{
    if (colony.getCount() == 0)
    {
        return;
    }
    if (!armed)
    {
        armed = true;
        armedRevision = colony.getRevision();
        sound.playMenu();
    }
    else
    {
        armed = false;
        colony.release(list.getSelected());
        sound.playLeave();
    }
    forceRefresh();
}
