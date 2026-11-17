#include "Arduboy2Core.h"
#include <esp_random.h>
#include <string.h>

// The OLED as the game sees it: SPItransfer() and paint8Pixels() write bytes one at a time, and a
// frame is handed to the host each time the cursor wraps.
static uint8_t vram[WIDTH * HEIGHT / 8];
static uint16_t cursor = 0;
static bool commandMode = false;
static uint8_t flags = 0;

static void pushFrame()
{
  abHostPaint(vram, flags);
}

static void setFlag(uint8_t flag, bool on)
{
  uint8_t next = on ? (flags | flag) : (flags & ~flag);
  if (next != flags) {
    flags = next;
    pushFrame();
  }
}

void Arduboy2Core::boot()
{
  memset(vram, 0, sizeof(vram));
  cursor = 0;
  commandMode = false;
  flags = 0;
}

void Arduboy2Core::safeMode() {}

void Arduboy2Core::idle()
{
  abHostIdle();
}

void Arduboy2Core::LCDDataMode()
{
  commandMode = false;
}

void Arduboy2Core::LCDCommandMode()
{
  commandMode = true;
}

void Arduboy2Core::SPItransfer(uint8_t data)
{
  if (commandMode) {
    sendLCDCommand(data);
    return;
  }
  vram[cursor++] = data;
  if (cursor == sizeof(vram)) {
    cursor = 0;
    pushFrame();
  }
}

uint8_t Arduboy2Core::SPItransferAndRead(uint8_t data)
{
  SPItransfer(data);
  return 0;
}

void Arduboy2Core::paint8Pixels(uint8_t pixels)
{
  bool wasCommand = commandMode;
  commandMode = false;
  SPItransfer(pixels);
  commandMode = wasCommand;
}

void Arduboy2Core::paintScreen(const uint8_t *image)
{
  memcpy(vram, image, sizeof(vram));
  cursor = 0;
  pushFrame();
}

void Arduboy2Core::paintScreen(uint8_t image[], bool clear)
{
  memcpy(vram, image, sizeof(vram));
  if (clear) {
    memset(image, 0, sizeof(vram));
  }
  cursor = 0;
  pushFrame();
}

void Arduboy2Core::blank()
{
  memset(vram, 0, sizeof(vram));
  cursor = 0;
  pushFrame();
}

void Arduboy2Core::sendLCDCommand(uint8_t command)
{
  switch (command) {
    case OLED_PIXELS_INVERTED: setFlag(AB_INVERT, true); break;
    case OLED_PIXELS_NORMAL: setFlag(AB_INVERT, false); break;
    case OLED_ALL_PIXELS_ON: setFlag(AB_ALL_ON, true); break;
    case OLED_PIXELS_FROM_RAM: setFlag(AB_ALL_ON, false); break;
    case OLED_VERTICAL_FLIPPED: setFlag(AB_FLIP_V, true); break;
    case OLED_VERTICAL_NORMAL: setFlag(AB_FLIP_V, false); break;
    case OLED_HORIZ_FLIPPED: setFlag(AB_FLIP_H, true); break;
    case OLED_HORIZ_NORMAL: setFlag(AB_FLIP_H, false); break;
    case 0xAE: setFlag(AB_OFF, true); break;
    case 0xAF: setFlag(AB_OFF, false); break;
    default: break;
  }
}

void Arduboy2Core::invert(bool inverse)
{
  setFlag(AB_INVERT, inverse);
}

void Arduboy2Core::allPixelsOn(bool on)
{
  setFlag(AB_ALL_ON, on);
}

void Arduboy2Core::flipVertical(bool flipped)
{
  setFlag(AB_FLIP_V, flipped);
}

void Arduboy2Core::flipHorizontal(bool flipped)
{
  setFlag(AB_FLIP_H, flipped);
}

void Arduboy2Core::displayOff()
{
  setFlag(AB_OFF, true);
}

void Arduboy2Core::displayOn()
{
  setFlag(AB_OFF, false);
}

uint8_t Arduboy2Core::buttonsState()
{
  return abHostButtons();
}

// The board has no RGB LED
void Arduboy2Core::setRGBled(uint8_t, uint8_t, uint8_t) {}
void Arduboy2Core::setRGBled(uint8_t, uint8_t) {}
void Arduboy2Core::freeRGBled() {}
void Arduboy2Core::digitalWriteRGB(uint8_t, uint8_t, uint8_t) {}
void Arduboy2Core::digitalWriteRGB(uint8_t, uint8_t) {}

unsigned long Arduboy2Core::generateRandomSeed()
{
  return esp_random();
}

void Arduboy2Core::delayShort(uint16_t ms)
{
  unsigned long start = millis();
  while (millis() - start < ms) {
    abHostIdle();
  }
}

void Arduboy2Core::exitToBootloader() {}
