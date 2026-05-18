#pragma once
#include "components/EnemyAI.h"
#include "components/BossPhaseData.h"
#include "data/BossAttackTable.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/Entity.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/resources/ResourceManager.h"
#include "engine/animation/AnimationClip.h"
#include <functional>
#include <glm/glm.hpp>

namespace sys {

using BossSpawnProjectileFn = std::function<eng::ecs::Entity(glm::vec2 from,
                                                              glm::vec2 vel,
                                                              float damage)>;
using OnBossPhaseChangeFn   = std::function<void(BossPhase prev, BossPhase next)>;
using OnBossDeathFn         = std::function<void(glm::vec3 pos)>;

// Returns the animation clip key for the given boss phase + EnemyState.
const char* bossClipName(BossPhase phase, EnemyState state);

// Update one MiniBoss entity.
// Must be called from EnemyAISystem when kind == MiniBoss.
void updateMiniBoss(
    eng::ecs::Entity              entity,
    eng::ecs::Registry&           reg,
    eng::physics::PhysicsWorld&   physics,
    glm::vec2                     playerPos,
    float                         dt,
    const data::BossAttackTable&  table,
    eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
    const BossSpawnProjectileFn&  spawnProjectile,
    const OnBossPhaseChangeFn&    onPhaseChange = {},
    const OnBossDeathFn&          onDeath       = {});

} // namespace sys
