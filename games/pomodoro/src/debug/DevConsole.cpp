#ifdef DEV_MODE

#include "DevConsole.h"

DevConsole::DevConsole(FocusTimer &timer, TFT_eSPI &tft, ScreenManager &screens)
    : timer(timer), tft(tft), screens(screens), length(0)
{
}

void DevConsole::update()
{
    while (Serial.available() > 0)
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (length > 0)
            {
                line[length] = '\0';
                length = 0;
                run(line);
            }
        }
        else if (length < sizeof(line) - 1)
        {
            line[length++] = c;
        }
    }
}

// The panel read back over MISO, row by row, as raw RGB565 after a "SHOT w h" line, for
// tools/screenshot.py. Blocks for the ~13 s it takes at 115200 baud.
void DevConsole::sendScreen()
{
    static uint16_t row[TFT_WIDTH];
    int16_t w = tft.width(), h = tft.height();
    Serial.printf("SHOT %d %d\n", w, h);
    for (int16_t y = 0; y < h; y++)
    {
        tft.readRect(0, y, w, 1, row);
        Serial.write((const uint8_t *)row, w * sizeof(uint16_t));
    }
    Serial.println();
    Serial.println("END");
}

void DevConsole::run(char *command)
{
    const char *name = strtok(command, " ");
    if (name == nullptr)
    {
        return;
    }
    const char *arg = strtok(nullptr, " ");

    if (strcmp(name, "end") == 0)
    {
        timer.finish(millis());
        Serial.println("phase ended");
    }
    else if (strcmp(name, "screen") == 0)
    {
        screens.setScreen(arg != nullptr ? atoi(arg) : 0);
        Serial.printf("screen: %d\n", screens.getCurrentIndex());
    }
    else if (strcmp(name, "shot") == 0)
    {
        sendScreen();
    }
    else
    {
        Serial.println("end | screen <i> | shot");
    }
}

#endif // DEV_MODE
