#include "WheelScreen.h"
#include <stdio.h>

static const int16_t WHEEL_Y = 32;
static const int16_t RESULT_Y = 214;
static const int16_t CAPTION_Y = 246;
static const unsigned long FRAME_MS = 33;
// Faster pegs blur into one click, as the buzzer cannot keep up with them
static const unsigned long CLICK_GAP_MS = 30;

WheelScreen::WheelScreen(TFT_eSPI &tft, Prefs &prefs, Spinner &spinner, History &history, RandomSource source,
                         SoundManager &sound, void (*saved)())
    : ToolScreen("Wheel", 1, sound, saved),
      wheel(tft, spinner, 120 - WheelView::SIZE / 2, WHEEL_Y),
      resultText(120, RESULT_Y, "", Theme::TEXT, 3, TC_DATUM),
      captionText(120, CAPTION_Y, "", Theme::TEXT_2, 1, TC_DATUM),
      prefs(prefs),
      spinner(spinner),
      history(history),
      source(source)
{
    addElement(&wheel);
    addElement(&resultText);
    addElement(&captionText);
    fields[0].setLabel("Slices");
    action.setLabel("Spin");
}

void WheelScreen::refreshFields()
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned)prefs.slices);
    fields[0].setValue(buf);
}

bool WheelScreen::changeField(uint8_t field, int16_t delta)
{
    int16_t next = prefs.slices + delta;
    next = next < MIN_SLICES ? MIN_SLICES : (next > MAX_SLICES ? MAX_SLICES : next);
    if (next == prefs.slices)
    {
        return false;
    }
    prefs.slices = (uint8_t)next;
    spinner.setSlices(prefs.slices);
    hasResult = false;
    wheel.setWinner(-1);
    wheel.markDirty();
    return true;
}

void WheelScreen::act()
{
    spinner.spin(millis(), source);
    heardCrossings = 0;
    hasResult = false;
    wheel.setWinner(-1);
    forceRefresh();

    char what[16];
    snprintf(what, sizeof(what), "Wheel of %u", (unsigned)spinner.getSlices());
    history.add(Tool::WHEEL, Mark::PLAIN, spinner.getResult() + 1, what, "", millis());
}

void WheelScreen::finish()
{
    hasResult = true;
    wheel.setWinner((int8_t)spinner.getResult());
    wheel.markDirty();
    sound.playResult();
    forceRefresh();
}

void WheelScreen::show()
{
    char buf[8];
    if (spinner.isSpinning())
    {
        snprintf(buf, sizeof(buf), "%u", (unsigned)spinner.sliceAtPointer() + 1);
        resultText.setColor(Theme::TEXT_DIM);
        resultText.setText(buf);
        captionText.setColor(Theme::TEXT_2);
        captionText.setText("Spinning...");
        return;
    }
    if (hasResult)
    {
        snprintf(buf, sizeof(buf), "%u", (unsigned)spinner.getResult() + 1);
        resultText.setColor(WheelView::sliceColor(spinner.getResult(), spinner.getSlices()));
        resultText.setText(buf);
    }
    else
    {
        resultText.setColor(Theme::TEXT_DIM);
        resultText.setText("--");
    }
    const char *caption = hint();
    captionText.setColor(caption != nullptr ? Theme::TEXT_DIM : Theme::TEXT_2);
    captionText.setText(caption != nullptr ? caption : "Confirm to spin");
}

void WheelScreen::update(unsigned long now)
{
    if (spinner.isSpinning())
    {
        if (now - lastFrame >= FRAME_MS)
        {
            lastFrame = now;
            spinner.update(now);
            wheel.markDirty();
            if (spinner.getCrossings() != heardCrossings && now - lastClick >= CLICK_GAP_MS)
            {
                heardCrossings = spinner.getCrossings();
                lastClick = now;
                sound.playClick();
            }
            if (!spinner.isSpinning())
            {
                finish();
            }
            show();
        }
        return;
    }
    if (!refreshDue(now))
    {
        return;
    }
    show();
}

void WheelScreen::onEnter(TFT_eSPI &tft)
{
    show();
    ToolScreen::onEnter(tft);
}

void WheelScreen::onExit()
{
    if (spinner.isSpinning())
    {
        spinner.stop();
        hasResult = true;
        wheel.setWinner((int8_t)spinner.getResult());
    }
    wheel.release();
    ToolScreen::onExit();
}
