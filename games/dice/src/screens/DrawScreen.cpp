#include "DrawScreen.h"
#include <stdio.h>

static const int16_t CARD_Y = 32;
static const int16_t CARD_H = 186;
static const int16_t NUMBER_Y = 42;
static const int16_t CAPTION_Y = 98;
static const int16_t CHIPS_Y = 118;
static const int16_t CHIP_GAP = 4;
static const uint8_t CHIPS_PER_ROW = 5;

static const unsigned long SHUFFLE_MS = 600;
static const unsigned long SHUFFLE_FRAME_MS = 50;

DrawScreen::DrawScreen(Prefs &prefs, Raffle &raffle, History &history, RandomSource source, SoundManager &sound,
                       void (*saved)())
    : ToolScreen("Draw", FIELD_TOTAL, sound, saved),
      card(5, CARD_Y, 230, CARD_H, Theme::FRAME, true, Theme::CARD, true, 4),
      numberText(120, NUMBER_Y, "", Theme::TEXT, 1, TC_DATUM, false, Theme::CARD),
      captionText(120, CAPTION_Y, "", Theme::TEXT_2, 1, TC_DATUM),
      prefs(prefs),
      raffle(raffle),
      history(history),
      source(source)
{
    addElement(&card);
    numberText.setFont(7);
    addElement(&numberText, Theme::CARD);
    addElement(&captionText, Theme::CARD);
    int16_t rowW = CHIPS_PER_ROW * Chip::W + (CHIPS_PER_ROW - 1) * CHIP_GAP;
    for (uint8_t i = 0; i < RECENT; i++)
    {
        chips[i].setPosition(120 - rowW / 2 + (i % CHIPS_PER_ROW) * (Chip::W + CHIP_GAP),
                             CHIPS_Y + (i / CHIPS_PER_ROW) * (Chip::H + CHIP_GAP));
        chips[i].setVisible(false);
        addElement(&chips[i], Theme::CARD);
    }
    fields[FIELD_FROM].setLabel("From");
    fields[FIELD_TO].setLabel("To");
    fields[FIELD_REPEATS].setLabel("Repeats");
}

void DrawScreen::applyRange()
{
    int16_t from = raffle.getFrom(), to = raffle.getTo();
    bool repeats = raffle.getRepeats();
    raffle.setRange(prefs.drawFrom, prefs.drawTo);
    raffle.setRepeats(prefs.drawRepeats != 0);
    // The hat was refilled
    if (from != raffle.getFrom() || to != raffle.getTo() || repeats != raffle.getRepeats())
    {
        clearRecent();
    }
}

void DrawScreen::clearRecent()
{
    recentCount = 0;
}

void DrawScreen::refreshFields()
{
    char buf[24];
    snprintf(buf, sizeof(buf), "%d", (int)prefs.drawFrom);
    fields[FIELD_FROM].setValue(buf);
    snprintf(buf, sizeof(buf), "%d", (int)prefs.drawTo);
    fields[FIELD_TO].setValue(buf);
    fields[FIELD_REPEATS].setValue(prefs.drawRepeats ? "Yes" : "No");
    action.setLabel(raffle.exhausted() ? "Put them back" : "Draw");
}

bool DrawScreen::changeField(uint8_t field, int16_t delta)
{
    if (field == FIELD_REPEATS)
    {
        prefs.drawRepeats = !prefs.drawRepeats;
        applyRange();
        return true;
    }
    int16_t &bound = field == FIELD_FROM ? prefs.drawFrom : prefs.drawTo;
    int32_t next = (int32_t)bound + delta;
    next = next < RAFFLE_MIN ? RAFFLE_MIN : (next > RAFFLE_MAX ? RAFFLE_MAX : next);
    if (next == bound)
    {
        return false;
    }
    bound = (int16_t)next;
    // The other bound gives way rather than the edit stopping
    if (prefs.drawFrom > prefs.drawTo)
    {
        if (field == FIELD_FROM)
        {
            prefs.drawTo = prefs.drawFrom;
        }
        else
        {
            prefs.drawFrom = prefs.drawTo;
        }
    }
    applyRange();
    return true;
}

// Tens below 100 and hundreds above, so both 1-75 and 1-1000 are a few presses away
int16_t DrawScreen::bigStep(uint8_t field, int8_t direction) const
{
    if (field == FIELD_REPEATS)
    {
        return 1;
    }
    int16_t value = field == FIELD_FROM ? prefs.drawFrom : prefs.drawTo;
    bool large = direction > 0 ? value >= 100 : value > 100;
    return large ? 100 : 10;
}

void DrawScreen::act()
{
    if (raffle.exhausted())
    {
        raffle.restart();
        clearRecent();
        refreshRows();
        sound.playMenu();
        return;
    }
    int16_t value = raffle.draw(source);
    for (uint8_t i = RECENT - 1; i > 0; i--)
    {
        recent[i] = recent[i - 1];
    }
    recent[0] = value;
    if (recentCount < RECENT)
    {
        recentCount++;
    }

    char what[16], detail[32];
    snprintf(what, sizeof(what), "%d-%d", (int)raffle.getFrom(), (int)raffle.getTo());
    if (raffle.getRepeats())
    {
        snprintf(detail, sizeof(detail), "Repeats allowed");
    }
    else
    {
        snprintf(detail, sizeof(detail), "%u of %u left", (unsigned)raffle.remaining(), (unsigned)raffle.size());
    }
    history.add(Tool::DRAW, Mark::PLAIN, value, what, detail, millis());

    shuffling = true;
    shuffleStart = millis();
    lastShuffleFrame = 0;
    forceRefresh();
}

void DrawScreen::settle()
{
    shuffling = false;
    if (raffle.exhausted())
    {
        sound.playEmpty();
    }
    else
    {
        sound.playResult();
    }
    refreshRows();
}

void DrawScreen::show()
{
    char buf[32];
    if (recentCount == 0)
    {
        numberText.setColor(Theme::TEXT_DIM);
        numberText.setText("--");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%d", (int)recent[0]);
        numberText.setColor(Theme::TEXT);
        numberText.setText(buf);
    }

    // The newest is the big number; the tokens begin with it so a glance reads the order
    for (uint8_t i = 0; i < RECENT; i++)
    {
        chips[i].setVisible(i < recentCount);
        if (i < recentCount)
        {
            chips[i].set(recent[i], i == 0);
        }
    }

    const char *caption = hint();
    Color color = Theme::TEXT_DIM;
    if (caption == nullptr)
    {
        color = Theme::TEXT_2;
        if (raffle.exhausted())
        {
            snprintf(buf, sizeof(buf), "All %u drawn", (unsigned)raffle.size());
            color = Theme::GOLD;
        }
        else if (raffle.getRepeats())
        {
            snprintf(buf, sizeof(buf), "Any of %u, repeats allowed", (unsigned)raffle.size());
        }
        else
        {
            snprintf(buf, sizeof(buf), "%u of %u left", (unsigned)raffle.remaining(), (unsigned)raffle.size());
        }
        caption = buf;
    }
    captionText.setColor(color);
    captionText.setText(caption);
}

void DrawScreen::update(unsigned long now)
{
    if (shuffling)
    {
        if (now - shuffleStart >= SHUFFLE_MS)
        {
            settle();
        }
        else
        {
            if (lastShuffleFrame == 0 || now - lastShuffleFrame >= SHUFFLE_FRAME_MS)
            {
                lastShuffleFrame = now;
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", (int)(raffle.getFrom() + uniform(source, raffle.size())));
                numberText.setColor(Theme::TEXT_DIM);
                numberText.setText(buf);
                captionText.setColor(Theme::TEXT_2);
                captionText.setText("Drawing...");
                sound.playClatter();
            }
            return;
        }
    }
    if (!refreshDue(now))
    {
        return;
    }
    show();
}

void DrawScreen::onEnter(TFT_eSPI &tft)
{
    show();
    ToolScreen::onEnter(tft);
}

void DrawScreen::onExit()
{
    if (shuffling)
    {
        shuffling = false;
        refreshFields();
    }
    ToolScreen::onExit();
}
