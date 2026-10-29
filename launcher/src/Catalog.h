#ifndef CATALOG_H
#define CATALOG_H

#include <Arduino.h>
#include <esp_partition.h>

// The site writes this to the `catalog` partition (type 0x40), little-endian:
// "GCAT", version, game count, two reserved bytes, then one CatalogEntry per game.
// An icon is THUMB_W x THUMB_H RGB565, high byte first, at thumbOffset from the
// partition's start. docs/bundle.js at the repo root is the writer; keep the two in step.
struct __attribute__((packed)) CatalogEntry
{
    char label[16];   // The game's app partition
    char name[24];
    char tagline[32]; // ASCII: the GLCD font has no accents
    uint16_t accent;  // RGB565
    uint8_t thumbW;
    uint8_t thumbH;
    uint32_t thumbOffset; // 0 for none
};
static_assert(sizeof(CatalogEntry) == 80, "the site writes 80-byte entries");

static const uint32_t CATALOG_MAGIC = 0x54414347; // "GCAT"
static const uint8_t CATALOG_VERSION = 2;
static const esp_partition_type_t CATALOG_TYPE = (esp_partition_type_t)0x40;
static const uint8_t THUMB_W = 64;
static const uint8_t THUMB_H = 64;

struct Game
{
    char name[25];
    char tagline[33];
    uint16_t accent;
    uint32_t thumbOffset;
    const esp_partition_t *app;
};

class Catalog
{
public:
    static const uint8_t MAX_GAMES = 16;

    // Without a catalog (a plain pio upload), lists every app partition but this one by label
    void load();

    uint8_t count() const { return gameCount; }
    const Game &game(uint8_t i) const { return games[i]; }
    int indexOf(const char *label) const;
    // `out` holds THUMB_W * THUMB_H pixels, ready for pushImage without byte swapping
    bool readThumb(uint8_t i, uint16_t *out) const;

private:
    bool loadCatalog();
    void loadPartitions();
    bool add(const esp_partition_t *app, const char *name, const char *tagline, uint16_t accent,
             uint32_t thumbOffset);

    const esp_partition_t *catalog = nullptr;
    Game games[MAX_GAMES];
    uint8_t gameCount = 0;
};

#endif // CATALOG_H
