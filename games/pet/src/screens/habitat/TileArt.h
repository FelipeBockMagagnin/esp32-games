#ifndef TILE_ART_H
#define TILE_ART_H

#include <TFT_eSPI.h>
#include "../../game/World.h"
#include "../../game/Colony.h"

// Drawn with primitives into a 16x16 sprite rather than stored as bitmaps: a tile icon set
// would cost flash for every kind, stage and species
namespace TileArt
{
    void paintTile(TFT_eSprite &s, const World &world, uint16_t i);
    // `dx` shifts it sideways, so two critters can share a tile; `bob` lifts it a pixel
    void paintCritter(TFT_eSprite &s, const Critter &c, int16_t dx, bool bob);
    void paintCursor(TFT_eSprite &s, uint16_t color);
}

#endif // TILE_ART_H
