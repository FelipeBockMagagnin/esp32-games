#include "TileArt.h"

using namespace TerrainArt;

// Scatters the grass blades and leaves the same way on every repaint of a tile
static uint32_t tileHash(uint16_t i, uint8_t salt)
{
    uint32_t h = (i + 1) * 2654435761u ^ (salt * 40503u);
    h ^= h >> 13;
    return h * 2246822519u;
}

static void paintGrass(TFT_eSprite &s, uint16_t i)
{
    s.fillSprite(GRASS);
    for (uint8_t b = 0; b < 5; b++)
    {
        uint32_t h = tileHash(i, b);
        int16_t x = h % 15, y = (h >> 8) % 14;
        s.drawFastVLine(x, y, 2, b % 2 ? GRASS_DARK : GRASS_LIGHT);
    }
}

static void paintWild(TFT_eSprite &s, uint16_t i)
{
    s.fillSprite(WILD);
    for (uint8_t b = 0; b < 4; b++)
    {
        uint32_t h = tileHash(i, b);
        s.fillCircle(2 + h % 12, 2 + (h >> 8) % 12, 3, WILD_LEAF);
    }
    uint32_t h = tileHash(i, 9);
    s.drawLine(h % 8, 15, 8 + (h >> 4) % 8, 2, WILD_THORN);
}

static void paintSoil(TFT_eSprite &s, bool wet)
{
    s.fillSprite(wet ? SOIL_WET : SOIL);
    for (int16_t y = 3; y < 16; y += 4)
    {
        s.drawFastHLine(1, y, 14, SOIL_DARK);
    }
}

static void paintCrop(TFT_eSprite &s, const World &world, uint16_t i)
{
    paintSoil(s, world.isWatered(i) && !world.isRipe(i));
    switch (world.cropStage(i))
    {
    case 0:
        s.fillRect(5, 9, 2, 2, LEAF);
        s.fillRect(10, 8, 2, 2, LEAF);
        break;
    case 1:
        s.drawFastVLine(8, 8, 5, SPROUT);
        s.drawPixel(7, 8, SPROUT);
        s.drawPixel(9, 7, SPROUT);
        break;
    case 2:
        s.drawFastVLine(8, 5, 8, LEAF);
        s.fillEllipse(5, 8, 3, 2, SPROUT);
        s.fillEllipse(11, 7, 3, 2, SPROUT);
        break;
    default:
        s.drawFastVLine(8, 3, 9, LEAF);
        s.fillEllipse(4, 9, 3, 2, SPROUT);
        s.fillEllipse(12, 9, 3, 2, SPROUT);
        s.fillCircle(8, 6, 3, CROPS[world.cropAt(i)].color);
        s.drawPixel(7, 5, 0xFFFF);
        break;
    }
}

void TileArt::paintTile(TFT_eSprite &s, const World &world, uint16_t i)
{
    switch (world.kindAt(i))
    {
    case TileKind::WILD:
        paintWild(s, i);
        break;
    case TileKind::SOIL:
        paintSoil(s, false);
        break;
    case TileKind::CROP:
        paintCrop(s, world, i);
        break;
    case TileKind::BURROW:
        paintGrass(s, i);
        s.fillEllipse(8, 10, 7, 5, MOUND);
        s.fillEllipse(9, 12, 5, 3, MOUND_DARK);
        s.fillEllipse(8, 11, 3, 2, HOLE);
        break;
    case TileKind::TROUGH:
        paintGrass(s, i);
        s.fillRect(1, 7, 14, 7, WOOD_DARK);
        s.fillRect(2, 8, 12, 5, WOOD);
        if (world.getFood() > 0)
        {
            s.fillRect(3, 8, 10, 2, FEED);
        }
        break;
    case TileKind::POND:
        s.fillSprite(GRASS);
        s.fillRoundRect(0, 1, 16, 14, 5, WATER);
        s.drawFastHLine(3, 5, 5, WATER_LIGHT);
        s.drawFastHLine(8, 10, 5, WATER_LIGHT);
        break;
    case TileKind::FLOWERS:
        paintGrass(s, i);
        for (uint8_t f = 0; f < 4; f++)
        {
            uint32_t h = tileHash(i, f + 20);
            int16_t x = 3 + (f % 2) * 8 + h % 3, y = 3 + (f / 2) * 8 + (h >> 4) % 3;
            uint16_t petal = PETALS[(h >> 8) % (sizeof(PETALS) / sizeof(PETALS[0]))];
            s.drawPixel(x - 1, y, petal);
            s.drawPixel(x + 1, y, petal);
            s.drawPixel(x, y - 1, petal);
            s.drawPixel(x, y + 1, petal);
            s.drawPixel(x, y, FEED);
        }
        break;
    default:
        paintGrass(s, i);
        break;
    }
}

void TileArt::paintCritter(TFT_eSprite &s, const Critter &c, int16_t dx, bool bob)
{
    const SpeciesDef &def = SPECIES[c.species];
    bool asleep = c.state == (uint8_t)CritterState::SLEEPING;
    int16_t cx = 8 + dx;
    int16_t lift = bob && !asleep ? 1 : 0;
    int16_t by = 11 - lift;

    switch (def.shape)
    {
    case CritterShape::ROUND_EARS:
        s.fillCircle(cx - 3, by - 5, 2, def.body);
        s.fillCircle(cx + 2, by - 5, 2, def.body);
        s.drawPixel(cx - 3, by - 5, def.accent);
        s.drawPixel(cx + 2, by - 5, def.accent);
        break;
    case CritterShape::LONG_EARS:
        s.fillRect(cx - 3, by - 9, 2, 6, def.body);
        s.fillRect(cx + 1, by - 9, 2, 6, def.body);
        s.drawFastVLine(cx - 3, by - 8, 4, def.accent);
        s.drawFastVLine(cx + 1, by - 8, 4, def.accent);
        break;
    case CritterShape::POINTY_EARS:
        s.fillTriangle(cx - 5, by - 3, cx - 3, by - 8, cx - 1, by - 3, def.body);
        s.fillTriangle(cx + 1, by - 3, cx + 3, by - 8, cx + 5, by - 3, def.body);
        s.fillTriangle(cx - 6, by + 3, cx - 9, by - 1, cx - 5, by, def.body);
        break;
    default:
        break;
    }
    s.fillEllipse(cx, by, 5, 4, def.body);
    s.fillEllipse(cx, by + 2, 3, 2, def.accent);
    if (def.shape == CritterShape::STRIPE)
    {
        s.drawFastHLine(cx - 3, by - 3, 7, def.accent);
    }
    if (def.shape == CritterShape::NO_EARS)
    {
        s.drawPixel(cx + 5, by, def.accent);
    }

    if (asleep)
    {
        s.drawFastHLine(cx + 1, by - 1, 2, EYE);
        s.drawFastHLine(cx + 4, 1, 3, SNORE);
        s.drawPixel(cx + 5, 2, SNORE);
        s.drawFastHLine(cx + 4, 3, 3, SNORE);
    }
    else
    {
        s.drawPixel(cx + 2, by - 1, EYE);
    }
    if (c.state == (uint8_t)CritterState::EATING)
    {
        s.drawPixel(cx + 5, by + 2, FEED);
        s.drawPixel(cx + 6, by + 1, FEED);
    }
}

void TileArt::paintCursor(TFT_eSprite &s, uint16_t color)
{
    s.drawRect(0, 0, 16, 16, color);
    s.drawRect(1, 1, 14, 14, color);
}
