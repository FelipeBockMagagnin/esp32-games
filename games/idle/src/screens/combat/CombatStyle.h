#ifndef COMBAT_STYLE_H
#define COMBAT_STYLE_H

#include "../../ui/Theme.h"

namespace CombatStyle
{
static constexpr Color READY = Theme::SELECT;  // Ring and outline of a pressable action
static constexpr Color ACTIVE = Theme::INFO;   // Guard and ward while they are up
static constexpr Color BLOCKED = Theme::BAD;
}

#endif // COMBAT_STYLE_H
