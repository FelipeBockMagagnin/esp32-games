#ifndef STUB_ESP_OTA_OPS_H
#define STUB_ESP_OTA_OPS_H

#include "esp_partition.h"

#define ESP_OK 0
inline int esp_ota_set_boot_partition(const esp_partition_t *) { return -1; }
inline void esp_restart() {}

#endif // STUB_ESP_OTA_OPS_H
