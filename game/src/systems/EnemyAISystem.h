#pragma once
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

void enemyAIUpdate(eng::ecs::Registry& reg,
                   eng::physics::PhysicsWorld& physics,
                   glm::vec2 playerPos,
                   float dt,
                   eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
                   const SpawnProjectileFn& spawnProjectile);

} // namespace sys
