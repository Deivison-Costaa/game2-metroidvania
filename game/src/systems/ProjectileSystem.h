#pragma once
#include "engine/ecs/Registry.h"
#include "engine/physics/PhysicsWorld.h"

namespace sys {

// Advance projectile lifetimes and destroy expired ones.
void projectileUpdate(eng::ecs::Registry& reg,
                      eng::physics::PhysicsWorld& physics,
                      float dt);

} // namespace sys
