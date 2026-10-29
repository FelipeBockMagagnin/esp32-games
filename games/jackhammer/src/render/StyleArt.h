#ifndef STYLE_ART_H
#define STYLE_ART_H

#include "../assets/Sprites.h"
#include "../jack/Config.h"

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
};

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
    {&ART_PLAYER_PIXEL, LIST(PIXEL_ENEMIES), LIST(PIXEL_BACKGROUNDS), nullptr, ICON_ENEMY_PIXEL},
    {&ART_PLAYER_PEN, LIST(PEN_ENEMIES), LIST(PEN_BACKGROUNDS), nullptr, ICON_ENEMY_PEN},
    // The Unity scene gave Watercolor the Pen colours
    {&ART_PLAYER_WATERCOLOR, LIST(WATERCOLOR_ENEMIES), LIST(PEN_BACKGROUNDS), nullptr, ICON_ENEMY_WATERCOLOR},
    {&ART_PLAYER_CREEPY, LIST(CREEPY_ENEMIES), LIST(BLACK_BACKGROUND), &BACKDROP_CREEPY, ICON_ENEMY_CREEPY},
    {&ART_PLAYER_NEON, LIST(NEON_ENEMIES), LIST(BLACK_BACKGROUND), nullptr, ICON_ENEMY_NEON_PINK},
    {&ART_PLAYER_EASTER, LIST(EASTER_ENEMIES), LIST(EASTER_BACKGROUNDS), nullptr, ICON_ENEMY_EASTER},
};
#undef LIST
static_assert(sizeof(STYLE_ART) / sizeof(StyleArt) == STYLE_COUNT, "One art row per STYLES row");

#endif // STYLE_ART_H
