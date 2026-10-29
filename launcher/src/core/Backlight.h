#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <Arduino.h>

// The display's backlight on PWM, dimmed after a while without a press. tone() drives LEDC
// channel 0 and channel 1 shares its timer, hence channel 4.
class Backlight
{
public:
    explicit Backlight(int pin) : pin(pin) {}

    void begin();
    void setFull();

    // True when the press only woke the screen, so it must not reach the game.
    // `dimAfter` 0 never dims.
    bool update(unsigned long now, bool pressed, bool held, unsigned long dimAfter);

private:
    static const uint8_t CHANNEL = 4;
    static const uint8_t FULL = 255;
    static const uint8_t DIM = 20;

    int pin;
    unsigned long lastInputAt = 0;
    bool dimmed = false;
};

#endif // BACKLIGHT_H
