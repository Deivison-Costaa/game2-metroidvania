#pragma once
#include "components/EnemyAI.h"
#include "engine/ecs/Registry.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/resources/ResourceManager.h"
#include "engine/animation/AnimationClip.h"
#include "engine/ecs/Entity.h"
#include <functional>
#include <glm/glm.hpp>

namespace sys {

using SpawnProjectileFn = std::function<eng::ecs::Entity(glm::vec2 from,
                                                          glm::vec2 vel,
                                                          float damage)>;

// Called when an enemy transitions to the Dead state (position, enemy kind int).
using OnEnemyDeathFn = std::function<void(glm::vec3, int)>;

void enemyAIUpdate(eng::ecs::Registry& reg,
                   eng::physics::PhysicsWorld& physics,
                   glm::vec2 playerPos,
                   float dt,
                   eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
                   const SpawnProjectileFn& spawnProjectile,
                   const OnEnemyDeathFn& onDeath = {});

} // namespace sys
