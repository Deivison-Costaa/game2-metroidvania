#pragma once
#include "engine/ecs/Registry.h"

namespace sys {

// Copies Box2D body positions back to Transform components after physics step.
void physicsSyncUpdate(eng::ecs::Registry& reg);

} // namespace sys
