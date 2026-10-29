#ifndef JACK_ROUND_H
#define JACK_ROUND_H

#include <stdint.h>
#include "Config.h"
#include "Rng.h"

// Sides and facings, clockwise from up, so turning clockwise adds one
enum Side : uint8_t
{
    SIDE_UP,
    SIDE_RIGHT,
    SIDE_DOWN,
    SIDE_LEFT,
    SIDE_COUNT
};

enum RoundEvent : uint8_t
{
    EVENT_KILL = 1,
    EVENT_DEAD = 2,
    EVENT_SPAWN = 4,
};

// One round of the Unity game: the jackhammer in the middle, enemies walking in from the four
// sides, a kill when one meets the head and the end when one reaches the body. Plain C++
// stepped at 60 Hz, so tools/test.cpp plays it on the PC.
class Round
{
public:
    static const uint8_t MAX_ENEMIES = 32;
    static const uint8_t MAX_MARKS = 8;

    struct Enemy
    {
        bool alive;
        uint8_t side;
        uint8_t look; // Picks the art variant and animation phase; no effect on play
        float distance; // From the centre, in units
    };

    // Where something just happened, for the screen's particles
    struct Mark
    {
        uint8_t side;
        uint8_t look;
        float distance;
    };

    void start(uint32_t seed);
    // After a death: the field is cleared and spawning starts over at the pace it had reached
    void revive();

    // +1 clockwise, -1 counterclockwise; the head can hit an enemy as it swings onto it
    void turn(int8_t direction);
    void step();

    // RoundEvent bits since the last call
    uint8_t takeEvents();
    // The kills and the death since the last clearMarks()
    uint8_t getMarkCount() const { return markCount; }
    const Mark &getMark(uint8_t i) const { return marks[i]; }
    void clearMarks() { markCount = 0; }

    bool isActive() const { return active; }
    bool isDead() const { return dead; }
    uint8_t getFacing() const { return facing; }
    uint32_t getPoints() const { return points; }
    uint32_t getMultiplier() const { return points / POINTS_PER_MULTIPLIER + 1; }
    uint32_t getEarned() const { return earned; }
    // The coins earned since the last takeUnbanked(), so a continued round is not banked twice
    uint32_t getUnbanked() const { return earned - banked; }
    uint32_t takeUnbanked()
    {
        uint32_t fresh = getUnbanked();
        banked = earned;
        return fresh;
    }
    bool hasContinued() const { return continued; }
    uint32_t getContinuePrice() const { return earned > CONTINUE_MIN_PRICE ? earned : CONTINUE_MIN_PRICE; }
    float getInterval() const { return interval; }
    float getSeconds() const { return seconds; }
    const Enemy &getEnemy(uint8_t i) const { return enemies[i]; }

private:
    Enemy enemies[MAX_ENEMIES];
    Mark marks[MAX_MARKS];
    uint8_t markCount = 0;
    uint8_t events = 0;
    Rng rng;
    bool active = false;
    bool dead = false;
    bool continued = false;
    uint8_t facing = SIDE_UP;
    uint32_t points = 0;
    uint32_t earned = 0;
    uint32_t banked = 0;
    float interval = START_INTERVAL;
    float spawnIn = 0;
    float harderIn = 0;
    float seconds = 0;

    void clearField();
    void harder();
    void spawn();
    void collide();
    void mark(const Enemy &e);
};

#endif // JACK_ROUND_H
