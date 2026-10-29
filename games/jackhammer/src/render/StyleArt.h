#ifndef STYLE_ART_H
#define STYLE_ART_H

#include "../assets/Sprites.h"
#include "../jack/Config.h"

enum class Blend : uint8_t
{
    OPAQUE, // The texture's alpha cut in half
    ALPHA,  // Mixed over what is behind by its alpha
    ADD,    // Added to what is behind, the Neon material's glow
};

// A point of a colour over lifetime, `at` the share of the particle's life
struct ColorKey
{
    float at;
    uint16_t color; // RGB565
};

// A Unity particle system, in its own units: speeds in units a second, sizes in units, times in
// seconds. Its shape threw particles in 3D; only their motion across the screen is kept. The
// last fields are its Color, Size and Clamp Velocity modules, for the systems that had them.
struct Emitter
{
    const ParticleTexture *texture;
    uint16_t tintFrom; // RGB565; each particle takes a colour between the two, times its texture
    uint16_t tintTo;
    float speedMin;
    float speedMax;
    float sizeMin;
    float sizeMax;
    float lifeMin;
    float lifeMax;
    float rateMin; // A second, while it runs
    float rateMax;
    uint8_t burst; // Thrown at once every `period` seconds
    float period;
    float coneDegrees; // 0 for a sphere
    Blend blend;
    const ColorKey *gradient; // Times the tint over the particle's life; nullptr keeps the tint
    uint8_t gradientCount;
    float shrinkFrom; // The share of life its size holds before falling linearly to sizeEnd
    float sizeEnd;
    float maxSpeed; // Units a second, 0 for no limit
    float dampen;   // The share of the speed above maxSpeed taken off each step
};

// What each STYLES row looks like. The background colours are the Unity ChangeBackgroundColor
// arrays in RGB565; a kill switches to another one. Creepy draws its backdrop instead.
struct StyleArt
{
    const Art *player;
    const Art *const *enemies; // One picked per enemy, from its look
    uint8_t enemyCount;
    const uint16_t *backgrounds;
    uint8_t backgroundCount;
    const Backdrop *backdrop;
    const uint16_t *icon;
    const Emitter *start; // Around the hammer on the title (StartParticleSystem*)
    const Emitter *death; // Where an enemy dies (DeadParticle*)
    const Glow *const *glows; // Carried by the hammer, behind it
    uint8_t glowCount;
};

static const uint16_t WHITE = 0xFFFF;
static const uint16_t RED = 0xF800;
static const uint16_t NEON_PINK = 0xFC5B; // DeadParticleNeon's F888DB
static const float CONE = 36.56669f;

static const ColorKey NEON_COLORS[] = {{0, 0xEA7D},     {0.168f, 0xEA9D}, {0.435f, 0x6FFF},
                                       {0.609f, 0x6FFF}, {0.832f, 0xF58C}, {1, 0xFD8B}};
static const ColorKey CREEPY_COLORS[] = {{0.506f, 0xF800}, {1, 0x0000}};
static const ColorKey CREEPY_DEATH_COLORS[] = {{0, 0xFFFF}, {0.362f, 0xF800}, {0.903f, 0xF800}, {1, 0x0000}};

#define PLAIN Blend::OPAQUE, nullptr, 0, 1, 1, 0, 0
// The Pixel, Pen, Watercolor and Easter start systems share one shape and pace
#define START(texture, tint) &texture, WHITE, tint, 3, 6, 0.3f, 0.4f, 5, 5, 10, 30, 0, 5, CONE
#define DEATH(texture, small, large) &texture, WHITE, RED, 5, 10, small, large, 5, 5, 0, 0, 30, 0, 0
static const Emitter PIXEL_START = {START(PARTICLE_SQUARE, RED), PLAIN};
static const Emitter PEN_START = {START(PARTICLE_PEN, WHITE), PLAIN};
static const Emitter WATERCOLOR_START = {START(PARTICLE_WATERCOLOR, WHITE), PLAIN};
static const Emitter CREEPY_START = {&PARTICLE_CIRCLE, WHITE, WHITE, 90, 200, 0.7f, 1.2f, 3, 5, 0, 10, 30, 5, CONE,
                                     Blend::ALPHA, CREEPY_COLORS, 2, 0, 0.14f, 1, 0.5f};
static const Emitter NEON_START = {&PARTICLE_CIRCLE, WHITE, WHITE, 4, 7, 0.1f, 0.5f, 2, 5, 0, 10, 40, 1, 0,
                                   Blend::ADD, NEON_COLORS, 6, 0.96f, 0, 0, 0};
// Twice the Unity size: at 0.3 units an egg was ~25 px on a phone, 7 here, and read as a dash
static const Emitter EASTER_START = {&PARTICLE_EGG, WHITE, RED, 3, 6, 0.6f, 0.8f, 5, 5, 10, 30, 0, 5, CONE,
                                     Blend::ALPHA, nullptr, 0, 0, 0.14f, 0, 0};
static const Emitter PIXEL_DEATH = {DEATH(PARTICLE_SQUARE, 0.2f, 0.3f), PLAIN};
static const Emitter PEN_DEATH = {DEATH(PARTICLE_PEN, 0.3f, 0.4f), PLAIN};
static const Emitter WATERCOLOR_DEATH = {DEATH(PARTICLE_WATERCOLOR, 0.3f, 0.4f), PLAIN};
static const Emitter CREEPY_DEATH = {&PARTICLE_CIRCLE, WHITE, WHITE, 30, 60, 0.3f, 0.8f, 3, 6, 0, 0, 40, 0, 0,
                                     Blend::ALPHA, CREEPY_DEATH_COLORS, 4, 0, 0, 1, 0.5f};
static const Emitter NEON_DEATH = {&PARTICLE_CIRCLE, WHITE, NEON_PINK, 5, 10, 0.1f, 0.5f, 3, 5, 0, 0, 30, 0, 0,
                                   Blend::ADD, nullptr, 0, 1, 1, 0, 0};
static const Emitter EASTER_DEATH = {DEATH(PARTICLE_EGG, 0.4f, 0.6f), Blend::ALPHA, nullptr, 0, 1, 1, 0, 0};
#undef PLAIN
#undef START
#undef DEATH

static const Glow *const CREEPY_GLOWS[] = {&GLOW_CREEPY_FOG, &GLOW_CREEPY_LIGHT};

static const Art *const PIXEL_ENEMIES[] = {&ART_ENEMY_PIXEL};
static const Art *const PEN_ENEMIES[] = {&ART_ENEMY_PEN};
static const Art *const WATERCOLOR_ENEMIES[] = {&ART_ENEMY_WATERCOLOR};
static const Art *const CREEPY_ENEMIES[] = {&ART_ENEMY_CREEPY};
static const Art *const NEON_ENEMIES[] = {&ART_ENEMY_NEON_PINK, &ART_ENEMY_NEON_CYAN, &ART_ENEMY_NEON_ORANGE};
static const Art *const EASTER_ENEMIES[] = {&ART_ENEMY_EASTER};

static const uint16_t PIXEL_BACKGROUNDS[] = {0x85E7, 0x4410, 0x64F5, 0xC393, 0xCD6E, 0xD6CE, 0xD6AF, 0xC274};
static const uint16_t PEN_BACKGROUNDS[] = {0x85E7, 0x4410, 0x64F5, 0xAB0C, 0xB4EB, 0xBB08, 0xD6AF, 0xC274};
static const uint16_t BLACK_BACKGROUND[] = {0x0000};
static const uint16_t EASTER_BACKGROUNDS[] = {0x4DB3, 0x65A9, 0x74F6, 0x7DF7};

#define LIST(a) a, sizeof(a) / sizeof(a[0])
static const StyleArt STYLE_ART[] = {
    {&ART_PLAYER_PIXEL, LIST(PIXEL_ENEMIES), LIST(PIXEL_BACKGROUNDS), nullptr, ICON_ENEMY_PIXEL, &PIXEL_START,
     &PIXEL_DEATH, nullptr, 0},
    {&ART_PLAYER_PEN, LIST(PEN_ENEMIES), LIST(PEN_BACKGROUNDS), nullptr, ICON_ENEMY_PEN, &PEN_START, &PEN_DEATH,
     nullptr, 0},
    // The Unity scene gave Watercolor the Pen colours
    {&ART_PLAYER_WATERCOLOR, LIST(WATERCOLOR_ENEMIES), LIST(PEN_BACKGROUNDS), nullptr, ICON_ENEMY_WATERCOLOR,
     &WATERCOLOR_START, &WATERCOLOR_DEATH, nullptr, 0},
    {&ART_PLAYER_CREEPY, LIST(CREEPY_ENEMIES), LIST(BLACK_BACKGROUND), &BACKDROP_CREEPY, ICON_ENEMY_CREEPY,
     &CREEPY_START, &CREEPY_DEATH, LIST(CREEPY_GLOWS)},
    {&ART_PLAYER_NEON, LIST(NEON_ENEMIES), LIST(BLACK_BACKGROUND), nullptr, ICON_ENEMY_NEON_PINK, &NEON_START,
     &NEON_DEATH, nullptr, 0},
    {&ART_PLAYER_EASTER, LIST(EASTER_ENEMIES), LIST(EASTER_BACKGROUNDS), nullptr, ICON_ENEMY_EASTER, &EASTER_START,
     &EASTER_DEATH, nullptr, 0},
};
#undef LIST
static_assert(sizeof(STYLE_ART) / sizeof(StyleArt) == STYLE_COUNT, "One art row per STYLES row");

#endif // STYLE_ART_H
