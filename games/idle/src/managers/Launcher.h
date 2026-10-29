#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <esp_ota_ops.h>
#include <esp_partition.h>

// The game menu the site installs beside several games (../esp32-launcher) as the factory app.
// A single-game install has none, and then the game is all there is.
inline const esp_partition_t *findLauncher()
{
    return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "launcher");
}

inline bool launcherInstalled()
{
    return findLauncher() != nullptr;
}

// From now on the board boots into the menu, until it starts a game again. Returns only when
// there is no menu to go to.
inline void returnToLauncher()
{
    const esp_partition_t *launcher = findLauncher();
    if (launcher != nullptr && esp_ota_set_boot_partition(launcher) == ESP_OK)
    {
        esp_restart();
    }
}

#endif // LAUNCHER_H
