#pragma once
#include "engine/ecs/Registry.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/render/ParticleSystem.h"

namespace sys {

// Advance projectile lifetimes, emit trail particles, and destroy expired projectiles.
// particles may be null — trail emission is skipped if so.
void projectileUpdate(eng::ecs::Registry& reg,
                      eng::physics::PhysicsWorld& physics,
                      eng::render::ParticleSystem* particles,
                      float dt);

} // namespace sys
