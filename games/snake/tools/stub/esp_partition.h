// No partitions on the PC, so no game menu either
#ifndef STUB_ESP_PARTITION_H
#define STUB_ESP_PARTITION_H

typedef struct esp_partition_t esp_partition_t;
enum
{
    ESP_PARTITION_TYPE_APP,
    ESP_PARTITION_SUBTYPE_APP_FACTORY
};
inline const esp_partition_t *esp_partition_find_first(int, int, const char *) { return nullptr; }

#endif // STUB_ESP_PARTITION_H
