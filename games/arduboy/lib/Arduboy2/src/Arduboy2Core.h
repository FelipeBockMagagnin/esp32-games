/**
 * @file Arduboy2Core.h
 * \brief
 * The Arduboy2Core class for the dotcompany board: Arduboy2's hardware layer, with the OLED,
 * the buttons, the LED and the timers handed to ArduboyHost.h, which the firmware implements.
 * The API is upstream's (Arduboy2 6.0.0, BSD-3-Clause, LICENSE.txt), so games compile unchanged.
 */

#ifndef ARDUBOY2_CORE_H
#define ARDUBOY2_CORE_H

#include <Arduino.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include "ArduboyHost.h"

#define ARDUBOY_10
#define ARDUBOY_CORE

#define RGB_ON LOW
#define RGB_OFF HIGH

#define RED_LED 10
#define GREEN_LED 11
#define BLUE_LED 9

#define LEFT_BUTTON _BV(5)
#define RIGHT_BUTTON _BV(6)
#define UP_BUTTON _BV(7)
#define DOWN_BUTTON _BV(4)
#define A_BUTTON _BV(3)
#define B_BUTTON _BV(2)

#define PIN_SPEAKER_1 5
#define PIN_SPEAKER_2 13

#define OLED_PIXELS_INVERTED 0xA7
#define OLED_PIXELS_NORMAL 0xA6
#define OLED_ALL_PIXELS_ON 0xA5
#define OLED_PIXELS_FROM_RAM 0xA4
#define OLED_VERTICAL_FLIPPED 0xC0
#define OLED_VERTICAL_NORMAL 0xC8
#define OLED_HORIZ_FLIPPED 0xA0
#define OLED_HORIZ_NORMAL 0xA1

#define WIDTH 128
#define HEIGHT 64

#define COLUMN_ADDRESS_END (WIDTH - 1) & 127
#define PAGE_ADDRESS_END ((HEIGHT/8)-1) & 7

#define TXLED0
#define TXLED1
#define RXLED0
#define RXLED1

#define ARDUBOY_NO_USB

class Arduboy2NoUSB
{
};

class Arduboy2Core : public Arduboy2NoUSB
{
  friend class Arduboy2Ex;

  public:
    static void idle();
    static void LCDDataMode();
    static void LCDCommandMode();
    static void SPItransfer(uint8_t data);
    static uint8_t SPItransferAndRead(uint8_t data);
    static void displayOff();
    static void displayOn();
    static constexpr uint8_t width() { return WIDTH; }
    static constexpr uint8_t height() { return HEIGHT; }
    static uint8_t buttonsState();
    static void paint8Pixels(uint8_t pixels);
    static void paintScreen(const uint8_t *image);
    static void paintScreen(uint8_t image[], bool clear = false);
    static void blank();
    static void invert(bool inverse);
    static void allPixelsOn(bool on);
    static void flipVertical(bool flipped);
    static void flipHorizontal(bool flipped);
    static void sendLCDCommand(uint8_t command);
    static void setRGBled(uint8_t red, uint8_t green, uint8_t blue);
    static void setRGBled(uint8_t color, uint8_t val);
    static void freeRGBled();
    static void digitalWriteRGB(uint8_t red, uint8_t green, uint8_t blue);
    static void digitalWriteRGB(uint8_t color, uint8_t val);
    static void boot();
    static void safeMode();
    static unsigned long generateRandomSeed();
    static void delayShort(uint16_t ms);
    static void exitToBootloader();

  protected:
    static void setCPUSpeed8MHz() {}
    static void bootSPI() {}
    static void bootOLED() {}
    static void bootPins() {}
    static void bootPowerSaving() {}
};

#endif
