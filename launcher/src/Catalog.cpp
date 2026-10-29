#include "Catalog.h"
#include <esp_ota_ops.h>

static const uint16_t DEFAULT_ACCENT = 0xF5A8;

static void copyField(char *out, size_t outSize, const char *in, size_t inSize)
{
    size_t n = strnlen(in, inSize);
    if (n >= outSize)
    {
        n = outSize - 1;
    }
    memcpy(out, in, n);
    out[n] = '\0';
}

void Catalog::load()
{
    gameCount = 0;
    if (!loadCatalog())
    {
        gameCount = 0;
        loadPartitions();
    }
}

// An entry whose app is missing or was never written is skipped, not shown as a dead slot
bool Catalog::add(const esp_partition_t *app, const char *name, const char *tagline, uint16_t accent,
                  uint32_t thumbOffset)
{
    esp_app_desc_t desc;
    if (app == nullptr || gameCount >= MAX_GAMES || esp_ota_get_partition_description(app, &desc) != ESP_OK)
    {
        return false;
    }
    Game &g = games[gameCount++];
    copyField(g.name, sizeof(g.name), name, strlen(name));
    copyField(g.tagline, sizeof(g.tagline), tagline, strlen(tagline));
    g.accent = accent;
    g.thumbOffset = thumbOffset;
    g.app = app;
    return true;
}

bool Catalog::loadCatalog()
{
    catalog = esp_partition_find_first(CATALOG_TYPE, ESP_PARTITION_SUBTYPE_ANY, "catalog");
    if (catalog == nullptr)
    {
        return false;
    }
    uint8_t header[8];
    if (esp_partition_read(catalog, 0, header, sizeof(header)) != ESP_OK)
    {
        return false;
    }
    uint32_t magic;
    memcpy(&magic, header, sizeof(magic));
    if (magic != CATALOG_MAGIC || header[4] != CATALOG_VERSION)
    {
        return false;
    }
    uint8_t entries = header[5];
    for (uint8_t i = 0; i < entries; i++)
    {
        CatalogEntry e;
        if (esp_partition_read(catalog, sizeof(header) + i * sizeof(e), &e, sizeof(e)) != ESP_OK)
        {
            return false;
        }
        char label[17];
        char name[25];
        char tagline[33];
        copyField(label, sizeof(label), e.label, sizeof(e.label));
        copyField(name, sizeof(name), e.name, sizeof(e.name));
        copyField(tagline, sizeof(tagline), e.tagline, sizeof(e.tagline));
        bool thumbOk = e.thumbW == THUMB_W && e.thumbH == THUMB_H &&
                       e.thumbOffset + THUMB_W * THUMB_H * 2 <= catalog->size;
        add(esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, label), name, tagline,
            e.accent, thumbOk ? e.thumbOffset : 0);
    }
    return gameCount > 0;
}

void Catalog::loadPartitions()
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, nullptr);
    for (; it != nullptr; it = esp_partition_next(it))
    {
        const esp_partition_t *app = esp_partition_get(it);
        if (app != running)
        {
            add(app, app->label, "", DEFAULT_ACCENT, 0);
        }
    }
    esp_partition_iterator_release(it);
}

int Catalog::indexOf(const char *label) const
{
    for (uint8_t i = 0; i < gameCount; i++)
    {
        if (strcmp(games[i].app->label, label) == 0)
        {
            return i;
        }
    }
    return -1;
}

bool Catalog::readThumb(uint8_t i, uint16_t *out) const
{
    if (catalog == nullptr || games[i].thumbOffset == 0)
    {
        return false;
    }
    return esp_partition_read(catalog, games[i].thumbOffset, out, THUMB_W * THUMB_H * 2) == ESP_OK;
}
