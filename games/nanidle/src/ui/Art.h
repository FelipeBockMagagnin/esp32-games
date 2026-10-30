#ifndef UI_ART_H
#define UI_ART_H

#include "../assets/Sprites.h"
#include "../nan/Config.h"

const SpriteDef *spriteFor(Art art);
const SpriteDef *backdropFor(Backdrop backdrop);
const uint16_t *iconFor(ItemArt art); // 16x16, black transparent

#endif // UI_ART_H
