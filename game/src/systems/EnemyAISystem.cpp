#include "systems/EnemyAISystem.h"
#include "components/EnemyAI.h"
#include "components/Hitbox.h"
#include "components/Hurtbox.h"
#include "components/Health.h"
#include "components/RigidBody.h"
#include "components/SpriteRenderer.h"
#include "components/Transform.h"
#include "components/Animator.h"
#include "engine/core/Log.h"
#include "engine/physics/PhysicsConstants.h"
#include "engine/physics/CollisionCategories.h"
#include "engine/core/Log.h"
#include <box2d/box2d.h>
#include <cmath>
#include <algorithm>

using namespace eng::physics;

namespace sys {

// Map (kind, state) to animation clip key
static const char* enemyClipName(EnemyKind kind, EnemyState state) {
    switch (kind) {
    case EnemyKind::Walker:
        switch (state) {
        case EnemyState::Patrol:  return "walker_patrol";
        case EnemyState::Chase:   return "walker_chase";
        case EnemyState::Attack:  return "walker_attack";
        case EnemyState::Hurt:    return "walker_hurt";
        case EnemyState::Dead:    return "walker_dead";
        }
        break;
    case EnemyKind::Flyer:
        switch (state) {
        case EnemyState::Patrol:  return "flyer_patrol";
        case EnemyState::Chase:   return "flyer_chase";
        case EnemyState::Attack:  return "flyer_attack";
        case EnemyState::Hurt:    return "flyer_hurt";
        case EnemyState::Dead:    return "flyer_dead";
        }
        break;
    case EnemyKind::Ranged:
        switch (state) {
        case EnemyState::Patrol:  return "ranged_idle";
        case EnemyState::Chase:   return "ranged_idle";
        case EnemyState::Attack:  return "ranged_attack";
        case EnemyState::Hurt:    return "ranged_hurt";
        case EnemyState::Dead:    return "ranged_dead";
        }
        break;
    }
    return "";
}

static void transitionState(EnemyAI& ai, EnemyState newState,
                             eng::ecs::Entity e, eng::ecs::Registry& reg,
                             eng::resources::ResourceManager<eng::animation::AnimationClip>& clips)
{
    if (newState == ai.state) return;
    ai.state     = newState;
    ai.stateTimer = 0.f;
    const char* key = enemyClipName(ai.kind, newState);
    if (*key && clips.has(key) && reg.has<Animator>(e)) {
        auto& anim    = reg.get<Animator>(e);
        anim.clip     = clips.get(key);
        anim.time     = 0.f;
        anim.frameIdx = 0;
        anim.finished = false;
    }
}

// ── Walker FSM ────────────────────────────────────────────────────────────────

static void updateWalker(eng::ecs::Entity e, EnemyAI& ai,
                          eng::ecs::Registry& reg,
                          eng::physics::PhysicsWorld& physics,
                          glm::vec2 playerPos, float dt,
                          eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
                          const SpawnProjectileFn&)
{
    if (!reg.has<RigidBody>(e)) return;
    auto& rb   = reg.get<RigidBody>(e);
    b2Body* body = rb.body;
    const glm::vec2 myPos = toGlm(body->GetPosition());

    // Line-of-sight raycast (horizontal)
    const glm::vec2 toPlayer = playerPos - myPos;
    const float dist = glm::length(toPlayer);

    ai.hasLoS = false;
    if (dist < ai.visionRange) {
        glm::vec2 dir = (dist > 0.001f) ? (toPlayer / dist) : glm::vec2{1,0};
        // Horizontal LoS ray (ignore vertical gap — realistic enough for a ground enemy)
        glm::vec2 rayEnd = myPos + dir * std::min(dist + 0.1f, ai.visionRange);
        auto hit = physics.raycastAny(myPos, rayEnd, body);
        ai.hasLoS = !hit.hit; // no obstacle in the way
    }

    ai.stateTimer += dt;
    ai.attackCooldown = std::max(0.f, ai.attackCooldown - dt);

    switch (ai.state) {
    case EnemyState::Patrol: {
        // Walk between patrolMinX and patrolMaxX
        float speed = ai.speed * static_cast<float>(ai.facing);
        b2Vec2 vel  = body->GetLinearVelocity();
        vel.x       = speed;
        body->SetLinearVelocity(vel);

        if (myPos.x >= ai.patrolMaxX) ai.facing = -1;
        if (myPos.x <= ai.patrolMinX) ai.facing =  1;

        if (reg.has<SpriteRenderer>(e))
            reg.get<SpriteRenderer>(e).flipX = (ai.facing < 0);

        if (ai.hasLoS && dist < ai.visionRange)
            transitionState(ai, EnemyState::Chase, e, reg, clips);
        break;
    }
    case EnemyState::Chase: {
        // Move toward player
        const float dir = (playerPos.x > myPos.x) ? 1.f : -1.f;
        ai.facing = (dir > 0) ? 1 : -1;
        b2Vec2 vel = body->GetLinearVelocity();
        vel.x      = dir * ai.speed * 1.5f; // chase faster than patrol
        body->SetLinearVelocity(vel);

        if (reg.has<SpriteRenderer>(e))
            reg.get<SpriteRenderer>(e).flipX = (ai.facing < 0);

        if (dist <= ai.attackRange && ai.attackCooldown <= 0.f)
            transitionState(ai, EnemyState::Attack, e, reg, clips);
        else if (!ai.hasLoS && ai.stateTimer > 2.f)
            transitionState(ai, EnemyState::Patrol, e, reg, clips);
        break;
    }
    case EnemyState::Attack: {
        // Stand still during attack swing
        b2Vec2 vel = body->GetLinearVelocity();
        vel.x      = 0.f;
        body->SetLinearVelocity(vel);

        // Attack animation lasts ~0.4s; then return to chase or patrol
        if (ai.stateTimer > 0.4f) {
            ai.attackCooldown = 1.2f;
            transitionState(ai, dist < ai.visionRange ? EnemyState::Chase
                                                       : EnemyState::Patrol,
                            e, reg, clips);
        }
        break;
    }
    case EnemyState::Hurt: {
        // Stagger for 0.3s
        if (ai.stateTimer > 0.3f)
            transitionState(ai, EnemyState::Chase, e, reg, clips);
        break;
    }
    case EnemyState::Dead: {
        // Slow down and wait to be despawned
        b2Vec2 vel = body->GetLinearVelocity();
        vel.x *= 0.8f;
        body->SetLinearVelocity(vel);
        break;
    }
    }
}

// ── Flyer FSM ─────────────────────────────────────────────────────────────────

static void updateFlyer(eng::ecs::Entity e, EnemyAI& ai,
                         eng::ecs::Registry& reg,
                         eng::physics::PhysicsWorld& /*physics*/,
                         glm::vec2 playerPos, float dt,
                         eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
                         const SpawnProjectileFn&)
{
    if (!reg.has<RigidBody>(e)) return;
    auto& rb     = reg.get<RigidBody>(e);
    b2Body* body = rb.body;
    const glm::vec2 myPos = toGlm(body->GetPosition());

    ai.sinePhase  += dt * 2.5f; // ~2.5 rad/s oscillation
    ai.stateTimer += dt;
    ai.attackCooldown = std::max(0.f, ai.attackCooldown - dt);

    const float dist = glm::length(playerPos - myPos);
    ai.hasLoS = (dist < ai.visionRange); // Flyer ignores obstacles for LoS

    switch (ai.state) {
    case EnemyState::Patrol: {
        // Hover in sinusoidal pattern
        b2Vec2 vel;
        vel.x = static_cast<float>(ai.facing) * ai.speed;
        vel.y = std::sin(ai.sinePhase) * 2.0f;
        body->SetLinearVelocity(vel);

        if (myPos.x >= ai.patrolMaxX) ai.facing = -1;
        if (myPos.x <= ai.patrolMinX) ai.facing =  1;

        if (reg.has<SpriteRenderer>(e))
            reg.get<SpriteRenderer>(e).flipX = (ai.facing < 0);

        if (ai.hasLoS)
            transitionState(ai, EnemyState::Chase, e, reg, clips);
        break;
    }
    case EnemyState::Chase: {
        // Fly directly toward player (8-directional)
        const glm::vec2 toPlayer = playerPos - myPos;
        const float d = glm::length(toPlayer);
        if (d > 0.01f) {
            const glm::vec2 dir = toPlayer / d;
            body->SetLinearVelocity(toB2(dir * ai.speed * 2.f));
            ai.facing = (dir.x > 0) ? 1 : -1;
        }
        if (reg.has<SpriteRenderer>(e))
            reg.get<SpriteRenderer>(e).flipX = (ai.facing < 0);

        if (dist <= ai.attackRange && ai.attackCooldown <= 0.f)
            transitionState(ai, EnemyState::Attack, e, reg, clips);
        else if (dist > ai.visionRange)
            transitionState(ai, EnemyState::Patrol, e, reg, clips);
        break;
    }
    case EnemyState::Attack: {
        if (ai.stateTimer > 0.35f) {
            ai.attackCooldown = 1.f;
            transitionState(ai, EnemyState::Chase, e, reg, clips);
        }
        break;
    }
    case EnemyState::Hurt: {
        if (ai.stateTimer > 0.25f)
            transitionState(ai, EnemyState::Chase, e, reg, clips);
        break;
    }
    case EnemyState::Dead: {
        b2Vec2 vel = body->GetLinearVelocity();
        vel.x *= 0.85f; vel.y *= 0.85f;
        body->SetLinearVelocity(vel);
        break;
    }
    }
}

// ── Ranged FSM ────────────────────────────────────────────────────────────────

static void updateRanged(eng::ecs::Entity e, EnemyAI& ai,
                          eng::ecs::Registry& reg,
                          eng::physics::PhysicsWorld& physics,
                          glm::vec2 playerPos, float dt,
                          eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
                          const SpawnProjectileFn& spawnProjectile)
{
    if (!reg.has<RigidBody>(e)) return;
    auto& rb     = reg.get<RigidBody>(e);
    b2Body* body = rb.body;
    const glm::vec2 myPos = toGlm(body->GetPosition());
    const float dist = glm::length(playerPos - myPos);

    // LoS via raycast (horizontal)
    ai.hasLoS = false;
    if (dist < ai.visionRange) {
        auto hit = physics.raycastAny(myPos, playerPos, body);
        ai.hasLoS = !hit.hit;
    }

    // Face player
    ai.facing = (playerPos.x > myPos.x) ? 1 : -1;
    if (reg.has<SpriteRenderer>(e))
        reg.get<SpriteRenderer>(e).flipX = (ai.facing < 0);

    // Stay still (Ranged doesn't move)
    body->SetLinearVelocity({0.f, body->GetLinearVelocity().y});

    ai.stateTimer  += dt;
    ai.fireTimer    = std::max(0.f, ai.fireTimer - dt);

    switch (ai.state) {
    case EnemyState::Patrol:
    case EnemyState::Chase:
        if (ai.hasLoS && dist < ai.visionRange && ai.fireTimer <= 0.f)
            transitionState(ai, EnemyState::Attack, e, reg, clips);
        break;
    case EnemyState::Attack:
        // Fire projectile at the start of the attack animation
        if (ai.stateTimer > 0.2f && ai.fireTimer <= 0.f) {
            const glm::vec2 toPlayer = playerPos - myPos;
            const float d = glm::length(toPlayer);
            if (d > 0.01f) {
                const glm::vec2 dir = toPlayer / d;
                spawnProjectile(myPos + dir * 0.6f, dir * 8.f, 1.f);
            }
            ai.fireTimer = ai.fireCooldown;
            transitionState(ai, EnemyState::Patrol, e, reg, clips);
        }
        break;
    case EnemyState::Hurt:
        if (ai.stateTimer > 0.3f)
            transitionState(ai, EnemyState::Patrol, e, reg, clips);
        break;
    case EnemyState::Dead:
        break;
    }
}

// ── Dead entity cleanup ───────────────────────────────────────────────────────

static constexpr float kDeadDespawnDelay = 1.2f; // seconds after death before destroy

// ── Main update ───────────────────────────────────────────────────────────────

void enemyAIUpdate(eng::ecs::Registry& reg,
                   eng::physics::PhysicsWorld& physics,
                   glm::vec2 playerPos,
                   float dt,
                   eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
                   const SpawnProjectileFn& spawnProjectile,
                   const OnEnemyDeathFn& onDeath)
{
    // Collect dead entities to destroy after iteration
    std::vector<eng::ecs::Entity> toDestroy;

    for (auto [e, ai, rb] : reg.view<EnemyAI, RigidBody>()) {
        // Skip if already dead and waiting for despawn
        if (ai.state == EnemyState::Dead) {
            ai.stateTimer += dt;
            if (ai.stateTimer > kDeadDespawnDelay)
                toDestroy.push_back(e);
            continue;
        }

        // Invulnerability after hurt (handled via Health)
        if (reg.has<Health>(e)) {
            auto& hp = reg.get<Health>(e);
            if (hp.dead && ai.state != EnemyState::Dead) {
                transitionState(ai, EnemyState::Dead, e, reg, clips);
                if (onDeath && reg.has<Transform>(e))
                    onDeath(reg.get<Transform>(e).position,
                            static_cast<int>(ai.kind));
            }
        }

        switch (ai.kind) {
        case EnemyKind::Walker:
            updateWalker(e, ai, reg, physics, playerPos, dt, clips, spawnProjectile);
            break;
        case EnemyKind::Flyer:
            updateFlyer(e, ai, reg, physics, playerPos, dt, clips, spawnProjectile);
            break;
        case EnemyKind::Ranged:
            updateRanged(e, ai, reg, physics, playerPos, dt, clips, spawnProjectile);
            break;
        }
    }

    // Destroy dead entities outside the iteration loop
    for (eng::ecs::Entity e : toDestroy) {
        if (reg.has<RigidBody>(e)) {
            auto& rb = reg.get<RigidBody>(e);
            physics.world().DestroyBody(rb.body);
        }
        reg.destroy(e);
    }
}

} // namespace sys
