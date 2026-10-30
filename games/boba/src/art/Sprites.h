#ifndef ART_SPRITES_H
#define ART_SPRITES_H

#include <stdint.h>
#include "../pet/Config.h"

// What a drawing shows right now. A form is one drawing, every pose comes from its marks:
//   #  always lit     a/b  lit on the first/second frame of a two-frame step
//   o  eyes open      c    eyes shut
//   m  mouth open     n    mouth closed
//   j  only when cheering, like raised arms
struct Pose
{
    bool second = false;
    bool eyesShut = false;
    bool mouthOpen = false;
    bool cheer = false;
};

struct Sprite
{
    uint8_t w;
    uint8_t h;
    const char *art; // w * h marks, row by row
};

inline bool spriteDot(char mark, const Pose &pose)
{
    switch (mark)
    {
    case '#':
        return true;
    case 'a':
        return !pose.second;
    case 'b':
        return pose.second;
    case 'o':
        return !pose.eyesShut;
    case 'c':
        return pose.eyesShut;
    case 'm':
        return pose.mouthOpen;
    case 'n':
        return !pose.mouthOpen;
    case 'j':
        return pose.cheer;
    default:
        return false;
    }
}

extern const Sprite FORM_ART[FORM_COUNT];
extern const Sprite ART_POOP;
extern const Sprite ART_SKULL;
extern const Sprite ART_GRAVE;
extern const Sprite ART_HALO;
extern const Sprite ART_WING;
extern const Sprite ART_MEAL;
extern const Sprite ART_SNACK;
extern const Sprite ART_HEART;
extern const Sprite ART_HEART_EMPTY;
extern const Sprite ART_SYRINGE;
extern const Sprite ART_SPARK;
extern const Sprite ART_ANGRY;

enum Icon : uint8_t
{
    ICON_FOOD,
    ICON_LIGHT,
    ICON_GAME,
    ICON_MEDICINE,
    ICON_BATH,
    ICON_METER,
    ICON_DISCIPLINE,
    ICON_ATTENTION,
    ICON_COUNT
};

extern const Sprite ICON_ART[ICON_COUNT];

#endif // ART_SPRITES_H
