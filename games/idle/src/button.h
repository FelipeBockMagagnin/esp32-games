#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

class Button
{
private:
    int pinNumber;
    unsigned long debounceDelay;
    int lastSteadyState = HIGH;
    int lastFlickerState = HIGH;
    unsigned long lastDebounceTime = 0;
    bool pressedEvent = false;

public:
    Button(int pinNumber, unsigned long debounceDelay = 50)
        : pinNumber(pinNumber), debounceDelay(debounceDelay), pressedEvent(false)
    {
    }

    void setup()
    {
        pinMode(pinNumber, INPUT_PULLUP);
    }

    void loop()
    {
        int currentState = digitalRead(pinNumber);

        if (currentState != lastFlickerState)
        {
            lastDebounceTime = millis();
            lastFlickerState = currentState;
        }

        if ((millis() - lastDebounceTime) > debounceDelay)
        {
            if (lastSteadyState == HIGH && currentState == LOW)
            {
                pressedEvent = true;
            }
            lastSteadyState = currentState;
        }
    }

    bool wasPressed()
    {
        if (pressedEvent)
        {
            pressedEvent = false;
            return true;
        }
        return false;
    }
};

#endif // BUTTON_H
