#pragma once
#include "engine/ecs/Registry.h"

namespace sys {

// Mirror of combatPreUpdate but for enemy entities.
// Gates each enemy's Hitbox.active by EnemyState::Attack + attack timer.
void enemyCombatPreUpdate(eng::ecs::Registry& reg, float dt);

} // namespace sys
