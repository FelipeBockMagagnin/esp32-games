#ifndef WORLD_H
#define WORLD_H

#include <Arduino.h>
#include "WorldConfig.h"

// The ground and the farm's stores: tiles, crops, coins, the pantry, and what the shop sold.
// Every tile that changes how it looks is flagged, for the habitat grid to repaint alone.
class World
{
public:
    struct Snapshot
    {
        uint64_t coins;
        uint32_t food;
        uint16_t coinFraction; // Thousandths
        uint16_t cleared;      // Wild tiles bought, which prices the next
        uint16_t upgrades;     // One bit per UPGRADES row
        uint8_t crops;         // One bit per CROPS row unlocked; Clover is always known
        uint8_t kind[TILE_COUNT];
        uint8_t crop[TILE_COUNT];
        uint8_t water[TILE_COUNT]; // Seconds left
        uint16_t growth[TILE_COUNT];
    };

    World();

    void update(unsigned long now);

    static uint16_t indexOf(uint8_t x, uint8_t y) { return y * GRID_COLS + x; }
    static bool inGrid(int16_t x, int16_t y) { return x >= 0 && y >= 0 && x < GRID_COLS && y < GRID_ROWS; }

    TileKind kindAt(uint16_t i) const { return (TileKind)kind[i]; }
    bool isWalkable(int16_t x, int16_t y) const;
    uint8_t cropAt(uint16_t i) const { return crop[i]; }
    uint8_t cropStage(uint16_t i) const;
    bool isRipe(uint16_t i) const { return kind[i] == (uint8_t)TileKind::CROP && growth[i] >= GROWTH_FULL; }
    // Hand-watered or next to a pond
    bool isWatered(uint16_t i) const;
    uint8_t growthPercent(uint16_t i) const { return (uint32_t)growth[i] * 100 / GROWTH_FULL; }

    uint16_t count(TileKind k) const;
    // Breadth-first over walkable ground from (x, y) to the nearest tile of kind k (a ripe
    // one, for crops with `ripe`), which need not be walkable itself. False when none can be
    // reached; otherwise (stepX, stepY) is the first step on the way, or (x, y) when there.
    bool findPath(TileKind k, uint8_t x, uint8_t y, uint8_t &stepX, uint8_t &stepY, bool ripe = false) const;
    uint8_t countNear(TileKind k, uint8_t x, uint8_t y, uint8_t range) const;

    bool canClear(uint16_t i) const;
    uint32_t clearPrice() const;
    bool clear(uint16_t i);
    uint32_t buildPrice(BuildId b) const;
    bool build(uint16_t i, BuildId b);
    bool plant(uint16_t i, uint8_t c);
    bool water(uint16_t i);
    // The crop grows back from a sprout. Refuses while the pantry has no room for it all.
    bool harvest(uint16_t i);
    uint32_t harvestFood(uint8_t c) const;
    // Back to grass, or a crop back to bare soil
    bool demolish(uint16_t i);

    uint64_t getCoins() const { return coins; }
    bool spend(uint32_t price);
    void creditCoins(uint64_t thousandths);

    uint32_t getFood() const { return food; }
    uint32_t getPantrySize() const;
    bool takeFood(uint32_t amount);

    bool isCropUnlocked(uint8_t c) const { return c == 0 || (cropFlags & (1u << c)) != 0; }
    bool unlockCrop(uint8_t c);
    bool hasUpgrade(uint8_t u) const { return (upgradeFlags & (1u << u)) != 0; }
    bool buyUpgrade(uint8_t u, uint8_t population);
    uint16_t getEffect(UpgradeEffect effect) const;

    // Changes on every action the player takes, for the autosave
    uint32_t getRevision() const { return revision; }

    void markChanged(uint16_t i) { changed[i / 8] |= 1 << (i % 8); }
    bool takeChanged(uint16_t i);
    bool anyChanged() const;
    void markAllChanged();

    void save(Snapshot &out) const;
    void load(const Snapshot &in);

private:
    uint8_t kind[TILE_COUNT];
    uint8_t crop[TILE_COUNT];
    uint8_t waterLeft[TILE_COUNT];
    uint16_t growth[TILE_COUNT];
    uint8_t changed[(TILE_COUNT + 7) / 8];

    uint64_t coins;
    uint16_t coinFraction;
    uint32_t food;
    uint16_t cleared;
    uint16_t upgradeFlags;
    uint8_t cropFlags;
    uint32_t revision;

    unsigned long nextTick;
    uint8_t tickInSecond;

    void reset();
    void tick();
    bool nearPond(uint16_t i) const;
    void markPondArea(uint16_t i);
    void markKind(TileKind k);
};

#endif // WORLD_H
