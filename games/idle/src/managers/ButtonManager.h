#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include <driver/gpio.h>

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
        : pinNumber(pinNumber), debounceDelay(debounceDelay)
    {
    }

    void setup()
    {
        pinMode(pinNumber, INPUT_PULLUP);
    }

    int getPin() const
    {
        return pinNumber;
    }

    void loop()
    {
        unsigned long now = millis();
        int currentState = digitalRead(pinNumber);

        if (currentState != lastFlickerState)
        {
            lastDebounceTime = now;
            lastFlickerState = currentState;
        }

        if ((now - lastDebounceTime) > debounceDelay)
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

#endif // BUTTON_MANAGER_H
