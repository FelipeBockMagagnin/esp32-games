#ifndef ART_IDLE_POSE_H
#define ART_IDLE_POSE_H

#include <math.h>
#include "Rig.h"
#include "../pet/Pet.h"

// The pet standing somewhere other than its room: breathing, blinking now and then, its face
// by its mood. Worked out from the time alone, so any screen can show it alive.
inline Pose idlePose(const Pet &pet, unsigned long now, float x, float ground, float scale)
{
    Pose p;
    p.x = x;
    p.ground = ground;
    p.scale = scale;
    float breath = sinf(now * 2 * (float)M_PI / 2300) * 0.03f;
    p.squashY = 1 + breath;
    p.squashX = 1 - breath * 0.7f;
    p.sway = sinf(now * 2 * (float)M_PI / 2600);
    p.blush = 0.6f;
    if (FORM_RIG[pet.getForm()].feature == Feature::CLOUD)
    {
        p.lift = (8 + sinf(now * 2 * (float)M_PI / 2200) * 3) * scale;
    }
    switch (pet.mood())
    {
    case Mood::ASLEEP:
        p.eye = Eye::CLOSED;
        p.mouth = Mouth::NONE;
        break;
    case Mood::SICK:
        p.eye = Eye::DIZZY;
        p.mouth = Mouth::FLAT;
        p.pale = true;
        p.blush = 0;
        break;
    case Mood::SAD:
        p.eye = Eye::SAD;
        p.mouth = Mouth::FROWN;
        p.blush = 0.2f;
        break;
    case Mood::HAPPY:
        p.mouth = Mouth::W;
        p.blush = 0.9f;
        break;
    default:
        p.mouth = Mouth::SMILE;
        break;
    }
    if (pet.isEgg())
    {
        p.eye = Eye::CLOSED;
        p.mouth = Mouth::NONE;
    }
    else if (p.eye == Eye::OPEN && now % 4100 < 130)
    {
        p.eye = Eye::BLINK;
    }
    return p;
}

#endif // ART_IDLE_POSE_H
