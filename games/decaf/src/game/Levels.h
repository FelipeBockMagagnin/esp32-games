#ifndef LEVELS_H
#define LEVELS_H

#include <Arduino.h>
#include "../assets/Sprites.h"

static const int16_t TILE = TILE_SIZE;
static const uint8_t MAP_ROWS = 14;
static const int16_t VIEW_W = 320;
static const int16_t VIEW_H = MAP_ROWS * TILE;

// Map legend, one char per 16x16 cell. Sprites stand on the bottom of their cell.
//   #  ground (autotiled)           =  stone block          -  desk, jumped through from below
//   X  filing cabinet (autotiled)   ^  spikes up            v  spikes down
//   A  arena entry gate, shut while the boss fights      E  exit gate, shut until he falls
//   S  start   C  checkpoint   W  walker   D  drunk walker
//   P  printer firing left (2x2, this is its bottom-left cell)   Q  printer firing right
//   B  crumbling platform (3 cells wide, from here)   b  small crumbling platform
//   K  boss   F  the coworker at the end   M  the broken coffee machine
enum class Tile : uint8_t
{
    EMPTY,
    GROUND,
    BLOCK,
    DESK,
    CABINET,
    SPIKE_UP,
    SPIKE_DOWN,
    ENTRY_GATE,
    EXIT_GATE,
    PRINTER // Solid and drawn by the printer itself
};

// Plain RGB565 art colours for the office behind the tiles
struct Backdrop
{
    uint16_t wallTop;
    uint16_t wallBottom;
    uint16_t ceiling;
    uint16_t light;
    uint16_t window;
    uint16_t windowGlow;
    uint16_t windowFrame;
    uint16_t partition;
    uint16_t partitionCap;
    uint16_t monitor;
    uint16_t monitorGlow;
    uint16_t pit;
};

struct LevelDef
{
    const char *floor;        // HUD and cards, 3 chars max
    const char *name;         // 12 chars max
    const char *circle;       // Intro card subtitle, 24 chars max
    const char *const *map;   // MAP_ROWS rows of the same length
    Backdrop backdrop;
    const SheetDef *boss;
    const char *const *dialogue; // Pairs of lines, 24 chars max each, shown two at a time
    uint8_t dialogueLines;
};

static const char *const LIMBO_MAP[MAP_ROWS] = {
    "#                                                                                                               #################                                        ######################              #",
    "#                                                                                                               #################                                         A                  E               #",
    "#                                                                                                               #################                      ##                 A                  E               #",
    "#                                                                                                               #################                      ##                 A                  E               #",
    "#                                                                                                               #################                      ##   ##            A                  E               #",
    "#                                                                                            B                  #################                      ##   ##            A                  E               #",
    "#                                                                           ########### B         b             #################                      ##   ##            A                  E               #",
    "#                                                            ==             ###########              B                                                 ##   ##            A                  E               #",
    "#         ==   ===                                ===                       ###########                                                     D          ##   ######        A   ---      ---   E               #",
    "#                                   W                                       ###########                           ---      ---            ----         ##   ######        A                  E               #",
    "#                          ##############   ####       ####                 ###########                                                                     ######        A                  E               #",
    "#  S                  W    ##############   ####       ####        W   W    ###########                     C           P         P     D         D         ######   C    A             K    E       F M     #",
    "#########################################   ####       ####      ########^^^###########                   ############^#######################################################################################",
    "#########################################   ####       ####      ######################                   ####################################################################################################",
};

// Bea guides the way down, as Beatrice guided Dante
static const char *const LIMBO_DIALOGUE[] = {
    "Coffee? Oh, honey.",   "This one broke too.",
    "HR won't buy another.", "Something about Q3.",
    "Try the floor below.",  "Mind the stairs.",
};

static constexpr LevelDef LEVELS[] = {
    {"B1", "LIMBO", "The first circle", LIMBO_MAP,
     {0x2124, 0x39C8, 0x10A2, 0xCE79, 0x4A8E, 0x7BD4, 0x18C4, 0x2966, 0x4229, 0x0861, 0x5D7D, 0x0841},
     &sheet_boss1, LIMBO_DIALOGUE, sizeof(LIMBO_DIALOGUE) / sizeof(LIMBO_DIALOGUE[0])},
};
static constexpr uint8_t LEVEL_COUNT = sizeof(LEVELS) / sizeof(LEVELS[0]);

// The floors still to build, for the card after the last one
struct FloorName
{
    const char *floor;
    const char *name;
};
static constexpr FloorName CIRCLES[] = {
    {"B1", "LIMBO"}, {"B2", "LUST"},     {"B3", "GLUTTONY"}, {"B4", "GREED"},     {"B5", "WRATH"},
    {"B6", "HERESY"}, {"B7", "VIOLENCE"}, {"B8", "FRAUD"},    {"B9", "TREACHERY"},
};
static constexpr uint8_t CIRCLE_COUNT = sizeof(CIRCLES) / sizeof(CIRCLES[0]);

#endif // LEVELS_H
