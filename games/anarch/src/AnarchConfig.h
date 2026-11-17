// The game's settings, shared by the firmware and the PC tools, then the game itself
#ifndef ANARCH_CONFIG_H
#define ANARCH_CONFIG_H

// The menu's Exit row, which the game shows only when this is true
extern bool launcherShown;

#define SFG_SCREEN_RESOLUTION_X 160
#define SFG_SCREEN_RESOLUTION_Y 120
#define SFG_FPS 30
#define SFG_RAYCASTING_SUBSAMPLE 1
#define SFG_DITHERED_SHADOW 1
#define SFG_DIMINISH_SPRITES 1
#define SFG_CAN_EXIT launcherShown

#include "anarch/game.h"

#endif // ANARCH_CONFIG_H
