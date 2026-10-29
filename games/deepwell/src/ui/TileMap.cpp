#include "TileMap.h"
#include "../assets/Sprites.h"

static const uint16_t *const TILE_PIXELS[] = {image_wall_pixels, image_floor_pixels, image_stairs_pixels};

static const uint16_t *const GLYPH_PIXELS[] = {
    nullptr,
    image_hero_pixels,
    image_small_rat_pixels,
    image_slime_pixels,
    image_frog_pixels,
    image_goblin_pixels,
    image_zombie_pixels,
    image_frog_wizard_pixels,
    image_potion_01a_pixels,
    image_coin_03a_pixels,
    image_sword_01a_pixels,
    image_sword_01c_pixels,
    image_sword_01d_pixels,
    image_sword_01e_pixels,
    image_armor_01a_pixels,
    image_armor_01c_pixels,
    image_armor_01e_pixels,
};
static_assert(sizeof(GLYPH_PIXELS) / sizeof(GLYPH_PIXELS[0]) == GLYPH_COUNT, "a sprite per Glyph");

void TileMap::paintCell(TFT_eSPI &tft, uint8_t cx, uint8_t cy, Tile tile, Glyph glyph)
{
    static uint16_t buffer[TILE * TILE];
    const uint16_t *base = TILE_PIXELS[tile];
    const uint16_t *over = GLYPH_PIXELS[glyph];
    for (int16_t i = 0; i < TILE * TILE; i++)
    {
        uint16_t top = over != nullptr ? pgm_read_word(over + i) : 0;
        buffer[i] = top != 0 ? top : pgm_read_word(base + i);
    }
    tft.pushImage(x + cx * TILE, y + cy * TILE, TILE, TILE, buffer);
}

void TileMap::draw(TFT_eSPI &tft)
{
    uint8_t glyphs[MAP_H][MAP_W] = {};
    if (run.isActive())
    {
        // Items under enemies, the player over everything
        for (uint8_t pass = 0; pass < 2; pass++)
        {
            Entity::Kind kind = pass == 0 ? Entity::ITEM : Entity::ENEMY;
            for (uint8_t i = 0; i < Run::MAX_ENTITIES; i++)
            {
                const Entity &e = run.getEntity(i);
                if (e.kind != kind)
                {
                    continue;
                }
                glyphs[e.y][e.x] = kind == Entity::ITEM ? ITEMS[e.type].glyph : ENEMIES[e.type].glyph;
            }
        }
        glyphs[run.getPlayerY()][run.getPlayerX()] = GLYPH_HERO;
    }

    const Map &map = run.getMap();
    bool all = !drawn;
    for (uint8_t cy = 0; cy < MAP_H; cy++)
    {
        for (uint8_t cx = 0; cx < MAP_W; cx++)
        {
            Tile tile = map.at(cx, cy);
            uint8_t code = tile * GLYPH_COUNT + glyphs[cy][cx];
            if (all || shown[cy][cx] != code)
            {
                paintCell(tft, cx, cy, tile, (Glyph)glyphs[cy][cx]);
                shown[cy][cx] = code;
            }
        }
    }
}
