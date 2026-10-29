#ifndef JACK_CONFIG_H
#define JACK_CONFIG_H

#include <stdint.h>

// Positions are Unity units, the Unity game's own numbers, drawn at PX_PER_UNIT
static constexpr float PX_PER_UNIT = 20.0f;
static constexpr int16_t ARENA_PX = 240;
static constexpr float STEP_SECONDS = 1.0f / 60.0f;

// enemy1.prefab's 1.1-unit box
static constexpr float ENEMY_HALF = 0.55f;
// The jackhammer's head box (main character.prefab) ends 1.57 units out along the way it
// faces; an enemy touching it dies
static constexpr float HEAD_REACH = 1.57f + ENEMY_HALF;
// DetectCollision, the small box at its middle; an enemy touching it ends the round
static constexpr float BODY_REACH = 0.068f + ENEMY_HALF;

// Every side spawns just outside the square arena. The Unity spawns sat at 7.5 (up), 7
// (down), 9.5 (right) and 9 (left) units, so the time to react depended on the side; here it
// does not, and the speed is scaled so it matches their average.
static constexpr float SPAWN_DISTANCE = ARENA_PX / 2 / PX_PER_UNIT + ENEMY_HALF;
static constexpr float UNITY_SPAWN_DISTANCE = (7.5f + 7.0f + 9.5f + 9.0f) / 4;
static constexpr float UNITY_ENEMY_SPEED = 5.0f;
static constexpr float ENEMY_SPEED =
    UNITY_ENEMY_SPEED * (SPAWN_DISTANCE - HEAD_REACH) / (UNITY_SPAWN_DISTANCE - HEAD_REACH);

// DifficultyManager: once at the start and then every second the spawn interval shrinks,
// fast down to 0.8 s and slowly after. Its speed ramp never reached the enemies (Enemy1 read
// initialEnemy1Speed), so the game as published kept them at one speed, and so does this.
static constexpr float FIRST_SPAWN_SECONDS = 0.8f;
static constexpr float START_INTERVAL = 1.3f;
static constexpr float FAST_UNTIL = 0.8f;
static constexpr float FAST_STEP = 0.06f;
static constexpr float SLOW_STEP = 0.006f;
static constexpr float DIFFICULTY_SECONDS = 1.0f;

// One coin per kill, times one more for every 10 points
static constexpr uint32_t POINTS_PER_MULTIPLIER = 10;

// Continuing after a death (an ad in the Unity game) costs what the round has earned so far,
// at least this, and only once per round
static constexpr uint32_t CONTINUE_MIN_PRICE = 20;

struct ShopItem
{
    const char *name; // 14 chars max
    const char *note; // 30 chars max
    uint16_t price;   // The Unity market's
};

// The Unity StyleManager's order, which the art tables (render/StyleArt.h) follow
static constexpr ShopItem STYLES[] = {
    {"Pixel", "Where it all started", 0},
    {"Pen", "Ink sketches", 200},
    {"Watercolor", "Soft and smudged", 1000},
    {"Creepy", "Don't look at the clown", 3333},
    {"Neon", "Three colours of trouble", 10000},
    {"Easter", "A bunny and its eggs", 2000},
};
static constexpr uint8_t STYLE_COUNT = sizeof(STYLES) / sizeof(STYLES[0]);

// Tunes for the buzzer, after the Unity soundtrack's moods (audio/Songs.h)
static constexpr ShopItem SONGS[] = {
    {"Adventure", "Happy 8-bit", 0},
    {"Spike 'em", "A riff to hammer to", 300},
    {"Orchestra", "Epic, on one buzzer", 1500},
    {"Haunted", "Something in the walls", 4000},
    {"Neon Drive", "Night arpeggios", 7000},
};
static constexpr uint8_t SONG_COUNT = sizeof(SONGS) / sizeof(SONGS[0]);

static_assert(STYLE_COUNT <= 8 && SONG_COUNT <= 8, "Profile keeps the owned ones in a byte");

#endif // JACK_CONFIG_H
