#include "TreeLines.h"

static const unsigned long FRAME_MS = 25;
static const int32_t SPARK_SPEED = 450; // Pixels per second
static const unsigned long SPARK_MIN_MS = 160;
static const int16_t SPARK_HALF = 1; // A 3x3 spark
static const uint8_t MAX_DEPTH = 4;   // How far into the gaps a stale line may reach

TreeLines::TreeLines(int16_t x, int16_t top, int16_t pitch, int16_t rowH, int16_t indent, uint8_t count,
                     uint16_t lineColor, uint16_t sparkColor)
    : UI(x, top - (pitch - rowH), 0, count * pitch), top(top), pitch(pitch), rowH(rowH), indent(indent),
      slotCount(count < MAX_SLOTS ? count : MAX_SLOTS), lineColor(lineColor), sparkColor(sparkColor),
      layoutChanged(true), sparking(false), arrived(false), sparkTo(0), sparkX0(0), sparkY0(0), sparkY1(0),
      sparkX1(0), sparkStart(0), sparkMs(0), sparkX(0), sparkY(0), drawnSparkX(0), drawnSparkY(0),
      sparkDrawn(false), now(0), lastFrame(0)
{
    for (uint8_t i = 0; i < MAX_SLOTS; i++)
    {
        slots[i] = {0, 0, false};
    }
}

void TreeLines::setSlot(uint8_t slot, uint8_t depth, uint8_t through, bool continues)
{
    if (slot >= slotCount)
    {
        return;
    }
    Slot &s = slots[slot];
    if (s.depth == depth && s.through == through && s.continues == continues)
    {
        return;
    }
    s = {depth, through, continues};
    layoutChanged = true;
    if (sparking)
    {
        sparking = false;
        arrived = true;
    }
    markDirty();
}

void TreeLines::spark(int8_t fromSlot, uint8_t toSlot)
{
    uint8_t depth = toSlot < slotCount ? slots[toSlot].depth : 0;
    sparkTo = toSlot;
    if (depth == 0)
    {
        arrived = true;
        return;
    }
    sparkX0 = columnX(depth - 1);
    sparkY0 = fromSlot < 0 ? y : rowY(fromSlot) + rowH + SPARK_HALF;
    sparkY1 = rowY(toSlot) + rowH / 2;
    sparkX1 = x + depth * indent - SPARK_HALF - 2;
    unsigned long ms = (unsigned long)((sparkY1 - sparkY0) + (sparkX1 - sparkX0)) * 1000 / SPARK_SPEED;
    sparkMs = ms > SPARK_MIN_MS ? ms : SPARK_MIN_MS;
    sparkStart = now;
    sparking = true;
    arrived = false;
    lastFrame = 0;
}

bool TreeLines::takeArrival()
{
    bool was = arrived;
    arrived = false;
    return was;
}

void TreeLines::update(unsigned long time)
{
    now = time;
    if (!sparking || now - lastFrame < FRAME_MS)
    {
        return;
    }
    lastFrame = now;

    unsigned long age = now - sparkStart;
    if (age >= sparkMs)
    {
        sparking = false;
        arrived = true;
        markDirty();
        return;
    }
    int32_t vertical = sparkY1 - sparkY0;
    int32_t travelled = (vertical + (sparkX1 - sparkX0)) * (int32_t)age / (int32_t)sparkMs;
    if (travelled < vertical)
    {
        sparkX = sparkX0;
        sparkY = sparkY0 + travelled;
    }
    else
    {
        sparkX = sparkX0 + (travelled - vertical);
        sparkY = sparkY1;
    }
    markDirty();
}

// Each row's own column runs from the gap above it to its middle, then turns into the row
void TreeLines::drawLines(TFT_eSPI &tft)
{
    int16_t gap = pitch - rowH;
    for (uint8_t i = 0; i < slotCount; i++)
    {
        const Slot &s = slots[i];
        if (s.depth == 0)
        {
            continue;
        }
        int16_t rowTop = rowY(i);
        for (uint8_t column = 0; column + 1 < s.depth; column++)
        {
            if (s.through & (1 << column))
            {
                tft.drawFastVLine(columnX(column), rowTop - gap, pitch, lineColor);
            }
        }
        int16_t cx = columnX(s.depth - 1);
        int16_t mid = rowTop + rowH / 2;
        int16_t bottom = s.continues ? rowTop + rowH : mid + 1;
        tft.drawFastVLine(cx, rowTop - gap, bottom - (rowTop - gap), lineColor);
        tft.drawFastHLine(cx, mid, x + s.depth * indent - cx - 1, lineColor);
    }
}

// The lines repaint over themselves without a flicker, so a moving spark only clears where it was
void TreeLines::draw(TFT_eSPI &tft)
{
    int16_t gap = pitch - rowH;
    if (layoutChanged || !drawn)
    {
        layoutChanged = false;
        for (uint8_t i = 0; i < slotCount; i++)
        {
            tft.fillRect(x, rowY(i) - gap, MAX_DEPTH * indent, gap, eraseColor);
            if (slots[i].depth > 0)
            {
                tft.fillRect(x, rowY(i), slots[i].depth * indent, rowH, eraseColor);
            }
        }
        sparkDrawn = false;
    }
    else if (sparkDrawn)
    {
        tft.fillRect(drawnSparkX - SPARK_HALF, drawnSparkY - SPARK_HALF, SPARK_HALF * 2 + 1, SPARK_HALF * 2 + 1,
                     eraseColor);
        sparkDrawn = false;
    }

    drawLines(tft);

    if (sparking)
    {
        tft.fillRect(sparkX - SPARK_HALF, sparkY - SPARK_HALF, SPARK_HALF * 2 + 1, SPARK_HALF * 2 + 1, sparkColor);
        tft.drawPixel(sparkX, sparkY, Theme::TEXT);
        drawnSparkX = sparkX;
        drawnSparkY = sparkY;
        sparkDrawn = true;
    }
}
