#ifndef ART_PET_VIEW_H
#define ART_PET_VIEW_H

#include "Rig.h"
#include "Particles.h"
#include "Room.h"
#include "Spring.h"
#include "../pet/Pet.h"

enum class Anim : uint8_t
{
    NONE,
    MEAL,
    SNACK,
    CUDDLE,
    CLEAN,
    MEDS,
    HATCH,
    EVOLVE,
    REFUSE,
    CHEER,
    WAKE,
    STARTLE,
    COUNT
};

// How the pet moves on the Home screen. A spring squashes and stretches its body, jumps fall
// back under gravity, and when no care animation plays it picks something to do by its mood:
// walking, sitting, looking around, yawning, dancing, nosing the plant, chasing a dust mote.
// Only what is seen lives here; the state is the Pet's.
class PetView
{
public:
    void reset(unsigned long now, const Pet &pet);
    void play(Anim anim, unsigned long now, const Pet &pet);
    bool isPlaying(unsigned long now) const { return anim != Anim::NONE && (long)(now - animEnd) < 0; }
    Anim playing() const { return anim; }
    unsigned long elapsed(unsigned long now) const { return now - animStart; }
    // Turns its eyes toward a point on the screen for a while: the icon the player picks
    void lookAt(float x, float y, unsigned long now, unsigned long ms = 1600);

    void update(unsigned long now, const Pet &pet);
    void draw(Canvas &c, const Pet &pet, unsigned long now) const;
    // After the room's dark: the z's of its sleep shine through
    void drawGlow(Canvas &c, unsigned long now) const { particles.draw(c, now, true); }
    const Room::State &room() const { return roomState; }
    float headY(const Pet &pet) const;
    float getX() const { return x; }

private:
    enum class Act : uint8_t
    {
        IDLE,
        WALK,
        SIT,
        LOOK,
        YAWN,
        DANCE,
        BOUNCE,
        SNIFF,
        CHASE,
        SIGH,
        DOZE
    };

    Anim anim = Anim::NONE;
    unsigned long animStart = 0;
    unsigned long animEnd = 0;
    uint8_t animStep = 0; // how many of the animation's timed beats have fired

    Act act = Act::IDLE;
    unsigned long actStart = 0;
    unsigned long actEnd = 0;
    float x = 120;
    float target = 120;
    bool facingLeft = false;
    float walkPhase = 0;

    Spring stretch = Spring(190, 13); // + taller, - squashed
    float lift = 0;
    float liftV = 0;
    bool airborne = false;
    unsigned long jumpAt = 0; // a jump waits out its crouch
    float jumpPower = 0;

    float lookX = 0, lookY = 0;
    float lookToX = 0, lookToY = 0;
    unsigned long lookUntil = 0;
    unsigned long nextGlance = 0;
    unsigned long blinkUntil = 0;
    unsigned long nextBlink = 0;
    unsigned long nextAmbient = 0;
    uint8_t ambientCount = 0;
    uint8_t seenFluff = 0;
    bool seenAsleep = false;

    unsigned long lastUpdate = 0;
    Room::State roomState = {0, false, 0, 0};
    Particles particles;

    float width(const Pet &pet) const;
    void hop(unsigned long now, float power, unsigned long crouchMs = 90);
    void kick(float v) { stretch.kick(v); }
    void choose(unsigned long now, const Pet &pet);
    void ambient(unsigned long now, const Pet &pet);
    void beats(unsigned long now, const Pet &pet);
    void burst(unsigned long now, float cx, float cy, uint8_t count, bool confetti);
    Pose pose(const Pet &pet, unsigned long now) const;
};

#endif // ART_PET_VIEW_H
