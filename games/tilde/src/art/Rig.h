#ifndef ART_RIG_H
#define ART_RIG_H

#include <stdint.h>
#include "../gfx/Canvas.h"
#include "../pet/Config.h"

enum class Feature : uint8_t
{
    SPOTS,   // the egg's
    ANTENNA, // a sprout on its head
    PLEAT,   // a dumpling's twist on top
    EARS,
    DRIPS,
    MOSS,
    CLOUD, // puffs on top, and it floats
    LEAVES
};

// A form as shapes: an ellipse of a body, a face on it, and the one thing that makes it itself.
// Sizes are pixels at scale 1, the Home room's.
struct FormRig
{
    uint8_t w;
    uint8_t h;
    uint8_t body; // colour slot
    uint8_t eyeGap; // between the eyes' centres
    uint8_t eyeSize;
    int8_t eyeY; // the eyes' height above the body's centre
    Feature feature;
    bool feet;
    uint16_t voice; // Hz: the first note of its chirp
};

extern const FormRig FORM_RIG[FORM_COUNT];

enum class Eye : uint8_t
{
    OPEN,
    WIDE,
    HAPPY,  // ^ ^
    CLOSED, // u u, asleep or content
    BLINK,
    SAD,
    DIZZY,
    SQUEEZE, // > <
    STAR,
    NONE
};

enum class Mouth : uint8_t
{
    SMILE,
    W,
    OPEN,
    FROWN,
    CHEW,
    POUT,
    FLAT,
    NONE
};

// How the pet looks this frame. The animator fills one in; drawPet only draws it.
struct Pose
{
    float x = 120;      // the body's centre
    float ground = 190; // where its feet rest
    float lift = 0;     // above the ground, for jumps and floating
    float squashX = 1;
    float squashY = 1;
    float lean = 0; // how far the top of the body shifts sideways, in pixels
    float lookX = 0; // -1..1, where the pupils point
    float lookY = 0;
    float scale = 1;
    float step = 0;  // -1..1 while walking: which foot is forward
    float sway = 0;  // -1..1 for ears, leaves and drips
    float blush = 0; // 0..1
    float puff = 0;  // 0..1, cheeks puffed in a pout
    float flash = 0; // 0..1 toward white, when it grows
    Eye eye = Eye::OPEN;
    Mouth mouth = Mouth::SMILE;
    bool pale = false;       // unwell
    bool silhouette = false; // an Album form not met yet
    bool shadow = true;
    bool mirror = false;
};

void drawPet(Canvas &c, FormId form, const Pose &pose);
// The box drawPet covers, for the tests and for placing things around the pet
void petBounds(FormId form, const Pose &pose, float &x0, float &y0, float &x1, float &y1);

#endif // ART_RIG_H
