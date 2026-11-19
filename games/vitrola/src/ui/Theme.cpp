#include "Theme.h"
#include "Themes.h"

namespace Theme
{
    const uint16_t *activePalette = THEMES[0].colors;
    static uint8_t active = 0;

    uint8_t getActive()
    {
        return active;
    }

    void setActive(uint8_t index)
    {
        active = index < THEME_COUNT ? index : 0;
        activePalette = THEMES[active].colors;
    }
}
