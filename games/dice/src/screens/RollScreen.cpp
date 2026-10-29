#include "RollScreen.h"
#include <stdio.h>

static const int16_t CARD_Y = 32;
static const int16_t CARD_H = 164;
static const int16_t TOTAL_Y = 40;
static const int16_t CAPTION_Y = 96;
static const int16_t DICE_GAP = 8;
static const uint8_t DICE_PER_ROW = 5;
static const int16_t ONE_ROW_Y = 133;
static const int16_t TWO_ROWS_Y = 112;
static const int16_t ROW_GAP = 6;
static const int16_t TABLE_Y = 108;
static const int16_t TABLE_H = 86;

static const unsigned long TUMBLE_MS = 700;
static const unsigned long TUMBLE_FRAME_MS = 60;

RollScreen::RollScreen(Prefs &prefs, History &history, RandomSource source, SoundManager &sound, void (*saved)())
    : ToolScreen("Roll", FIELD_TOTAL, sound, saved),
      card(5, CARD_Y, 230, CARD_H, Theme::FRAME, true, Theme::CARD, true, 4),
      table(7, TABLE_Y, 226, TABLE_H, Theme::CARD, true, Theme::CARD),
      totalText(120, TOTAL_Y, "", Theme::TEXT, 1, TC_DATUM, false, Theme::CARD),
      captionText(120, CAPTION_Y, "", Theme::TEXT_2, 1, TC_DATUM),
      prefs(prefs),
      history(history),
      source(source)
{
    addElement(&card);
    table.setShowBorder(false);
    addElement(&table, Theme::CARD);
    totalText.setFont(7);
    addElement(&totalText, Theme::CARD);
    addElement(&captionText, Theme::CARD);
    for (DieFace &die : dice)
    {
        die.setVisible(false);
        addElement(&die, Theme::CARD);
    }
    fields[FIELD_COUNT].setLabel("Dice");
    fields[FIELD_DIE].setLabel("Die");
    fields[FIELD_BONUS].setLabel("Bonus");
    fields[FIELD_KEEP].setLabel("Mode");
}

void RollScreen::refreshFields()
{
    const RollSpec &spec = prefs.roll;
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", (unsigned)spec.count);
    fields[FIELD_COUNT].setValue(buf);
    snprintf(buf, sizeof(buf), "d%u", (unsigned)sidesOf(spec));
    fields[FIELD_DIE].setValue(buf);
    snprintf(buf, sizeof(buf), spec.bonus == 0 ? "0" : "%+d", (int)spec.bonus);
    fields[FIELD_BONUS].setValue(buf);
    fields[FIELD_KEEP].setValue(keepName((Keep)spec.keep));
    fields[FIELD_KEEP].setMuted(spec.count < 2);

    char formula[20];
    formatSpec(spec, formula, sizeof(formula));
    snprintf(buf, sizeof(buf), "Roll %s", formula);
    action.setLabel(buf);
}

static bool stepClamped(int16_t &value, int16_t delta, int16_t low, int16_t high)
{
    int16_t next = value + delta;
    next = next < low ? low : (next > high ? high : next);
    if (next == value)
    {
        return false;
    }
    value = next;
    return true;
}

static int16_t wrapped(int16_t value, int16_t delta, int16_t count)
{
    return (int16_t)(((value + delta) % count + count) % count);
}

bool RollScreen::changeField(uint8_t field, int16_t delta)
{
    RollSpec &spec = prefs.roll;
    int16_t v;
    switch (field)
    {
    case FIELD_COUNT:
        v = spec.count;
        if (!stepClamped(v, delta, 1, MAX_DICE))
        {
            return false;
        }
        spec.count = (uint8_t)v;
        break;
    case FIELD_DIE:
        spec.die = (uint8_t)wrapped(spec.die, delta, DIE_TYPE_COUNT);
        break;
    case FIELD_BONUS:
        v = spec.bonus;
        if (!stepClamped(v, delta, -MAX_BONUS, MAX_BONUS))
        {
            return false;
        }
        spec.bonus = (int8_t)v;
        break;
    default:
        spec.keep = (uint8_t)wrapped(spec.keep, delta, (int16_t)Keep::COUNT);
        break;
    }
    hasResult = false;
    layoutDice();
    return true;
}

int16_t RollScreen::bigStep(uint8_t field, int8_t direction) const
{
    return field == FIELD_BONUS ? 5 : 1;
}

void RollScreen::layoutDice()
{
    uint8_t count = hasResult || tumbling ? result.spec.count : prefs.roll.count;
    uint8_t top = count < DICE_PER_ROW ? count : DICE_PER_ROW;
    bool moved = false;
    int16_t xs[MAX_DICE], ys[MAX_DICE];
    for (uint8_t i = 0; i < MAX_DICE; i++)
    {
        xs[i] = dice[i].x;
        ys[i] = dice[i].y;
        if (i >= count)
        {
            moved |= dice[i].isVisible();
            continue;
        }
        uint8_t row = i / DICE_PER_ROW;
        uint8_t inRow = row == 0 ? top : count - top;
        int16_t rowW = inRow * DieFace::SIZE + (inRow - 1) * DICE_GAP;
        xs[i] = 120 - rowW / 2 + (i % DICE_PER_ROW) * (DieFace::SIZE + DICE_GAP);
        ys[i] = count <= DICE_PER_ROW ? ONE_ROW_Y : TWO_ROWS_Y + row * (DieFace::SIZE + ROW_GAP);
        moved |= !dice[i].isVisible() || xs[i] != dice[i].x || ys[i] != dice[i].y;
    }
    if (!moved)
    {
        return;
    }
    table.markDirty();
    for (uint8_t i = 0; i < MAX_DICE; i++)
    {
        dice[i].resetDrawn();
        dice[i].setVisible(i < count);
        dice[i].setPosition(xs[i], ys[i]);
    }
}

void RollScreen::act()
{
    rollDice(prefs.roll, source, result);
    char what[16], detail[32];
    formatSpec(result.spec, what, sizeof(what));
    formatFaces(result, detail, sizeof(detail));
    Mark mark = Mark::PLAIN;
    if (result.outcome == Outcome::CRITICAL || result.outcome == Outcome::MAX)
    {
        mark = Mark::GOOD;
    }
    else if (result.outcome == Outcome::FUMBLE || result.outcome == Outcome::MIN)
    {
        mark = Mark::BAD;
    }
    history.add(Tool::ROLL, mark, result.total, what, detail, millis());

    hasResult = false;
    tumbling = true;
    tumbleStart = millis();
    lastTumbleFrame = 0;
    layoutDice();
    forceRefresh();
}

void RollScreen::settle()
{
    tumbling = false;
    hasResult = true;
    switch (result.outcome)
    {
    case Outcome::CRITICAL:
        sound.playCritical();
        break;
    case Outcome::FUMBLE:
        sound.playFumble();
        break;
    default:
        sound.playResult();
        break;
    }
    forceRefresh();
}

void RollScreen::showTumble()
{
    uint8_t sides = sidesOf(result.spec);
    int16_t sum = result.spec.bonus;
    for (uint8_t i = 0; i < result.spec.count; i++)
    {
        uint8_t face = (uint8_t)(1 + uniform(source, sides));
        sum += face;
        dice[i].set(sides, face, DieFace::ROLLING);
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)sum);
    totalText.setColor(Theme::TEXT_DIM);
    totalText.setText(buf);
    captionText.setColor(Theme::TEXT_2);
    captionText.setText("Rolling...");
    sound.playClatter();
}

void RollScreen::showResult()
{
    const char *caption = hint();
    Color captionColor = Theme::TEXT_DIM;
    if (!hasResult)
    {
        uint8_t sides = sidesOf(prefs.roll);
        for (uint8_t i = 0; i < prefs.roll.count; i++)
        {
            dice[i].set(sides, 0, DieFace::KEPT);
        }
        totalText.setColor(Theme::TEXT_DIM);
        totalText.setText("--");
        captionText.setColor(captionColor);
        captionText.setText(caption != nullptr ? caption : "Confirm to roll");
        return;
    }

    DieFace::Style lit = DieFace::KEPT;
    Color totalColor = Theme::TEXT;
    const char *verdict = nullptr;
    switch (result.outcome)
    {
    case Outcome::CRITICAL:
        lit = DieFace::GOOD;
        totalColor = Theme::GOOD;
        verdict = "CRITICAL!";
        break;
    case Outcome::FUMBLE:
        lit = DieFace::BAD;
        totalColor = Theme::BAD;
        verdict = "FUMBLE";
        break;
    case Outcome::MAX:
        lit = DieFace::GOOD;
        totalColor = Theme::GOOD;
        verdict = "Highest possible";
        break;
    case Outcome::MIN:
        lit = DieFace::BAD;
        totalColor = Theme::BAD;
        verdict = "Lowest possible";
        break;
    default:
        break;
    }
    uint8_t sides = sidesOf(result.spec);
    for (uint8_t i = 0; i < result.spec.count; i++)
    {
        dice[i].set(sides, result.faces[i], result.kept[i] ? lit : DieFace::ASIDE);
    }
    char buf[24];
    snprintf(buf, sizeof(buf), "%d", (int)result.total);
    totalText.setColor(totalColor);
    totalText.setText(buf);

    if (caption == nullptr && verdict != nullptr)
    {
        caption = verdict;
        captionColor = totalColor;
    }
    else if (caption == nullptr)
    {
        formatSpec(result.spec, buf, sizeof(buf));
        caption = buf;
        captionColor = Theme::TEXT_2;
    }
    captionText.setColor(captionColor);
    captionText.setText(caption);
}

void RollScreen::update(unsigned long now)
{
    if (tumbling)
    {
        if (now - tumbleStart >= TUMBLE_MS)
        {
            settle();
        }
        else if (lastTumbleFrame == 0 || now - lastTumbleFrame >= TUMBLE_FRAME_MS)
        {
            lastTumbleFrame = now;
            showTumble();
            return;
        }
        else
        {
            return;
        }
    }
    if (!refreshDue(now))
    {
        return;
    }
    layoutDice();
    showResult();
}

void RollScreen::onEnter(TFT_eSPI &tft)
{
    layoutDice();
    showResult();
    ToolScreen::onEnter(tft);
}

void RollScreen::onExit()
{
    if (tumbling)
    {
        tumbling = false;
        hasResult = true;
    }
    ToolScreen::onExit();
}
