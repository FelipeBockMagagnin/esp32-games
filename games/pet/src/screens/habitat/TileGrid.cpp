#include "TileGrid.h"
#include "TileArt.h"

static const unsigned long BOB_MS = 600;

TileGrid::TileGrid(int16_t x, int16_t y, World &world, const Colony &colony)
    : UI(x, y, GRID_COLS * TILE_SIZE, GRID_ROWS * TILE_SIZE), world(world), colony(colony), sprite(nullptr),
      cursorX(0), cursorY(0), bob(false), nextBob(0)
{
}

void TileGrid::setCursor(uint8_t cx, uint8_t cy)
{
    if (cx == cursorX && cy == cursorY)
    {
        return;
    }
    world.markChanged(World::indexOf(cursorX, cursorY));
    cursorX = cx;
    cursorY = cy;
    world.markChanged(World::indexOf(cx, cy));
    markDirty();
}

void TileGrid::update(unsigned long now)
{
    if ((long)(now - nextBob) >= 0)
    {
        nextBob = now + BOB_MS;
        bob = !bob;
        for (uint8_t i = 0; i < colony.getCount(); i++)
        {
            const Critter &c = colony.get(i);
            if (c.state != (uint8_t)CritterState::SLEEPING)
            {
                world.markChanged(World::indexOf(c.x, c.y));
            }
        }
    }
    if (world.anyChanged())
    {
        markDirty();
    }
}

void TileGrid::paint(TFT_eSPI &tft, uint16_t i)
{
    TileArt::paintTile(*sprite, world, i);
    uint8_t tx = i % GRID_COLS, ty = i / GRID_COLS;
    uint8_t here = 0;
    for (uint8_t c = 0; c < colony.getCount() && here < 2; c++)
    {
        const Critter &critter = colony.get(c);
        if (critter.x == tx && critter.y == ty)
        {
            // A second one on the tile stands a step behind and out of step with the first
            TileArt::paintCritter(*sprite, critter, here == 0 ? 0 : -4, bob != (here == 1));
            here++;
        }
    }
    if (tx == cursorX && ty == cursorY)
    {
        TileArt::paintCursor(*sprite, Theme::SELECT);
    }
    sprite->pushSprite(x + tx * TILE_SIZE, y + ty * TILE_SIZE);
}

void TileGrid::draw(TFT_eSPI &tft)
{
    if (sprite == nullptr)
    {
        sprite = new TFT_eSprite(&tft);
        sprite->setColorDepth(16);
        if (sprite->createSprite(TILE_SIZE, TILE_SIZE) == nullptr)
        {
            delete sprite;
            sprite = nullptr;
            return;
        }
    }
    // Entering the screen or a repaint cleared it: every tile goes out again
    if (!drawn)
    {
        world.markAllChanged();
    }
    for (uint16_t i = 0; i < TILE_COUNT; i++)
    {
        if (world.takeChanged(i))
        {
            paint(tft, i);
        }
    }
}
