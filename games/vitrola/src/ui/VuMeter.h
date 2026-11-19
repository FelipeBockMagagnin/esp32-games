#ifndef VU_METER_H
#define VU_METER_H

#include <TFT_eSPI.h>
#include "../audio/Player.h"

// A row of LED bars, one per pitch band of the player; each bar repaints only the segments that
// changed
class VuMeter
{
public:
    static const int16_t Y = 262;
    static const int16_t H = 32;

    void reset();
    void draw(TFT_eSPI &tft, const Player &player);

private:
    static const uint8_t SEGMENTS = 8;
    static const int16_t BAR_W = 12;
    static const int16_t PITCH = 15;
    static const int16_t X0 = 2;
    uint8_t drawn[Player::BANDS];
};

#endif // VU_METER_H
