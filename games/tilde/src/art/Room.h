#ifndef ART_ROOM_H
#define ART_ROOM_H

#include "../gfx/Canvas.h"
#include "../pet/Pet.h"

// The room on the Home screen: a rounded card with a window, a picture, a hanging lamp, a
// plant and a rug on a wooden floor. Everything in it moves a little.
namespace Room
{
    static const int16_t X = 6;
    static const int16_t Y = 30;
    static const int16_t W = 228;
    static const int16_t H = 184;
    static const int16_t FLOOR_Y = 166;
    static const int16_t GROUND = 196; // where the pet's feet rest
    static const int16_t WALK_LEFT = 30;
    static const int16_t WALK_RIGHT = 192;
    static const int16_t PLANT_X = 210;
    static const int16_t LAMP_X = 172;

    struct State
    {
        float night;          // 0 lit .. 1 dark: the lamp off, faded in
        bool stars;           // the window's night sky: it is asleep
        unsigned long lampAt; // when the lamp last went on, for its flicker
        unsigned long pokeAt; // when the pet last nosed the plant
    };

    // Under the pet
    void drawBack(Canvas &c, const Pet &pet, const State &s, unsigned long now);
    // Over the pet and its particles: the dark, then what glows through it
    void drawNight(Canvas &c, const State &s, unsigned long now);
    float fluffX(uint8_t spot);
}

#endif // ART_ROOM_H
