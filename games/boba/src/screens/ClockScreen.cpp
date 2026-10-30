#include "ClockScreen.h"
#include <stdio.h>

static const unsigned long BLINK_MS = 400;

ClockScreen::ClockScreen(Pet &pet, LcdMatrix &lcd, Panel &panel, void (*changed)(), ScreenManager &screenManager,
                         SoundManager &sound)
    : pet(pet), lcd(lcd), panel(panel), changed(changed), screenManager(screenManager), sound(sound)
{
}

void ClockScreen::onEnter(TFT_eSPI &tft)
{
    uint32_t seconds = pet.clockSeconds();
    hour = (int8_t)(seconds / 3600);
    minute = (int8_t)(seconds / 60 % 60);
    onMinutes = false;
    lcd.setInverted(false);
    panel.paint(tft);
    drawMatrix();
    lcd.present(tft);
}

void ClockScreen::drawMatrix()
{
    char hours[5];
    char minutes[5];
    snprintf(hours, sizeof(hours), "%02d", hour);
    snprintf(minutes, sizeof(minutes), "%02d", minute);
    lcd.clear();
    lcd.textCentered(1, "CLOCK");
    if (onMinutes || !blink)
    {
        lcd.text(6, 9, hours);
    }
    lcd.text(14, 9, ":");
    if (!onMinutes || !blink)
    {
        lcd.text(18, 9, minutes);
    }
}

void ClockScreen::update(unsigned long now)
{
    if ((long)(now - nextBlinkAt) >= 0)
    {
        nextBlinkAt = now + BLINK_MS;
        blink = !blink;
        drawMatrix();
    }
}

void ClockScreen::render(TFT_eSPI &tft)
{
    lcd.present(tft);
}

void ClockScreen::step(int8_t delta)
{
    if (onMinutes)
    {
        minute = (int8_t)((minute + 60 + delta) % 60);
    }
    else
    {
        hour = (int8_t)((hour + 24 + delta) % 24);
    }
    blink = false;
    nextBlinkAt = millis() + BLINK_MS;
    drawMatrix();
}

void ClockScreen::onUpPress()
{
    step(1);
    sound.playMenu();
}

void ClockScreen::onDownPress()
{
    step(-1);
    sound.playMenu();
}

bool ClockScreen::onRepeat(Key key)
{
    step(key == Key::UP ? 1 : -1);
    return true;
}

void ClockScreen::onConfirmPress()
{
    sound.playMenu();
    if (!onMinutes)
    {
        onMinutes = true;
        return;
    }
    pet.setClock((uint32_t)hour * 3600 + (uint32_t)minute * 60);
    changed();
    screenManager.closeModal();
}
