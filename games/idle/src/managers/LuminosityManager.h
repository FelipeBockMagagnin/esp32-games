#ifndef LUMINOSITY_MANAGER_H
#define LUMINOSITY_MANAGER_H

#include <Arduino.h>

class LuminosityManager
{
private:
    static const unsigned long READ_INTERVAL_MS = 500;
    static const int ADC_MAX = 4095;

    int pin;
    unsigned long lastMs = 0;
    int raw = 0;
    float smoothed = 0.0f;
    bool firstRead = true;

public:
    LuminosityManager(int pin) : pin(pin) {}

    void setup()
    {
        pinMode(pin, INPUT);
    }

    void loop(unsigned long now)
    {
        if (lastMs != 0 && now - lastMs < READ_INTERVAL_MS)
        {
            return;
        }
        lastMs = now;

        raw = analogRead(pin);

        // A bare LDR divider jitters by tens of counts, so the value is averaged
        if (firstRead)
        {
            smoothed = raw;
            firstRead = false;
        }
        else
        {
            smoothed = (smoothed * 0.7f) + (raw * 0.3f);
        }

        Serial.printf("Luminosidade: %d (%d%%)\n", raw, getPercent());
    }

    int getRaw() const { return (int)(smoothed + 0.5f); }

    // This divider reads high in the dark, so the scale is flipped: 0% dark, 100% bright
    int getPercent() const
    {
        int value = 100 - (int)((smoothed * 100.0f) / ADC_MAX + 0.5f);
        return constrain(value, 0, 100);
    }

    String getPercentText() const { return String(getPercent()) + "%"; }
};

#endif // LUMINOSITY_MANAGER_H
