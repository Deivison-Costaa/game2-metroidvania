#include "systems/BossLogic.h"
#include "components/Health.h"
#include "components/RigidBody.h"
#include "components/SpriteRenderer.h"
#include "components/Transform.h"
#include "components/Animator.h"
#include "components/Hitbox.h"
#include "engine/physics/PhysicsConstants.h"
#include "engine/physics/CollisionCategories.h"
#include <box2d/box2d.h>
#include <cmath>
#include <algorithm>

using namespace eng::physics;

namespace sys {

// ── Clip mapping ──────────────────────────────────────────────────────────────

const char* bossClipName(BossPhase phase, EnemyState state) {
    if (state == EnemyState::Hurt) return "boss_hurt";
    if (state == EnemyState::Dead) return "boss_dead";

    switch (phase) {
    case BossPhase::Stance:
        switch (state) {
        case EnemyState::Patrol:
        case EnemyState::Chase:   return "boss_stance_idle";
        case EnemyState::Attack:  return "boss_stance_slash";
        default: break;
        }
        break;
    case BossPhase::Enrage:
        switch (state) {
        case EnemyState::Patrol:
        case EnemyState::Chase:   return "boss_stance_idle";
        case EnemyState::Attack:  return "boss_enrage_slash";
        default: break;
        }
        break;
    case BossPhase::Desperate:
        switch (state) {
        case EnemyState::Patrol:
        case EnemyState::Chase:   return "boss_stance_idle";
        case EnemyState::Attack:  return "boss_desperate_radial";
        default: break;
        }
        break;
    case BossPhase::Transitioning:
        return "boss_hurt"; // Reuse hurt clip for transition flash
    }
    return "boss_stance_idle";
}

// ── Internal helpers ──────────────────────────────────────────────────────────

static constexpr float kTransitionDuration = 0.30f;
static constexpr float kVisionRange        = 12.f;
static constexpr float kAttackRange        = 1.8f;
static constexpr float kBaseSpeed          = 3.0f;
static constexpr float kAttackCooldown     = 1.2f;

static void setClip(eng::ecs::Entity e,
                    eng::ecs::Registry& reg,
                    const char* key,
                    eng::resources::ResourceManager<eng::animation::AnimationClip>& clips)
{
    if (!*key || !clips.has(key)) return;
    if (!reg.has<Animator>(e)) return;
    auto& anim    = reg.get<Animator>(e);
    anim.clip     = clips.get(key);
    anim.time     = 0.f;
    anim.frameIdx = 0;
    anim.finished = false;
}

static void changeState(EnemyAI& ai, BossPhaseData& bpd,
                        EnemyState newState, BossPhase phase,
                        eng::ecs::Entity e, eng::ecs::Registry& reg,
                        eng::resources::ResourceManager<eng::animation::AnimationClip>& clips)
{
    if (ai.state == newState) return;
    ai.state      = newState;
    ai.stateTimer = 0.f;
    bpd.windowTimer = 0.f;
    bpd.radialFired = false;
    setClip(e, reg, bossClipName(phase, newState), clips);
}

// Project direction to 8-direction unit vectors for radial fire
static glm::vec2 dirFromAngle(float radians) {
    return {std::cos(radians), std::sin(radians)};
}

// ── Main boss update ──────────────────────────────────────────────────────────

void updateMiniBoss(
    eng::ecs::Entity              entity,
    eng::ecs::Registry&           reg,
    eng::physics::PhysicsWorld&   physics,
    glm::vec2                     playerPos,
    float                         dt,
    const data::BossAttackTable&  table,
    eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
    const BossSpawnProjectileFn&  spawnProjectile,
    const OnBossPhaseChangeFn&    onPhaseChange,
    const OnBossDeathFn&          onDeath)
{
    if (!reg.valid(entity)) return;
    if (!reg.has<EnemyAI>(entity) || !reg.has<BossPhaseData>(entity)) return;

    auto& ai  = reg.get<EnemyAI>(entity);
    auto& bpd = reg.get<BossPhaseData>(entity);

    // Dead state — just wait for despawn (handled by GameScene)
    if (ai.state == EnemyState::Dead) {
        ai.stateTimer += dt;
        return;
    }

    // ── Health / phase threshold check ───────────────────────────────────────
    if (reg.has<Health>(entity)) {
        auto& hp = reg.get<Health>(entity);
        if (hp.dead && ai.state != EnemyState::Dead) {
            changeState(ai, bpd, EnemyState::Dead, bpd.currentPhase, entity, reg, clips);
            if (onDeath && reg.has<Transform>(entity))
                onDeath(reg.get<Transform>(entity).position);
            return;
        }

        const float hpFrac = hp.max > 0.f ? hp.current / hp.max : 0.f;

        // Phase transition check (only when not already transitioning)
        if (bpd.currentPhase != BossPhase::Transitioning) {
            BossPhase desired = BossPhase::Stance;
            if (hpFrac <= bpd.desperateThreshold)
                desired = BossPhase::Desperate;
            else if (hpFrac <= bpd.enrageThreshold)
                desired = BossPhase::Enrage;

            if (desired != bpd.currentPhase) {
                bpd.nextPhase       = desired;
                bpd.currentPhase    = BossPhase::Transitioning;
                bpd.transitionTimer = kTransitionDuration;
                bpd.windowTimer     = 0.f;
                bpd.dashing         = false;

                // Freeze movement
                if (reg.has<RigidBody>(entity)) {
                    auto& rb = reg.get<RigidBody>(entity);
                    if (rb.body) rb.body->SetLinearVelocity({0.f, 0.f});
                }
                setClip(entity, reg, "boss_hurt", clips);

                if (onPhaseChange)
                    onPhaseChange(bpd.currentPhase, desired);
                return;
            }
        }
    }

    // ── Transitioning ─────────────────────────────────────────────────────────
    if (bpd.currentPhase == BossPhase::Transitioning) {
        bpd.transitionTimer -= dt;
        if (bpd.transitionTimer <= 0.f) {
            bpd.currentPhase = bpd.nextPhase;
            bpd.attackWindowIdx = 0;
            changeState(ai, bpd, EnemyState::Chase, bpd.currentPhase, entity, reg, clips);
        }
        return;
    }

    if (!reg.has<RigidBody>(entity)) return;
    auto& rb = reg.get<RigidBody>(entity);
    if (!rb.body) return;

    ai.stateTimer += dt;

    const glm::vec2 bossPos2 = reg.has<Transform>(entity)
        ? glm::vec2{reg.get<Transform>(entity).position}
        : glm::vec2{0.f};
    const glm::vec2 toPlayer = playerPos - bossPos2;
    const float dist         = glm::length(toPlayer);
    const float speed        = [&]{
        const auto* pd = table.phaseByFraction(1.f); // dummy, use current phase
        (void)pd;
        switch(bpd.currentPhase) {
        case BossPhase::Enrage:
        case BossPhase::Desperate: return kBaseSpeed * 1.5f;
        default:                   return kBaseSpeed;
        }
    }();

    b2Vec2 vel = rb.body->GetLinearVelocity();

    switch (ai.state) {
    // ── Chase ─────────────────────────────────────────────────────────────────
    case EnemyState::Patrol:
    case EnemyState::Chase: {
        // Move toward player
        if (dist > kAttackRange) {
            const float dir = (toPlayer.x > 0.f) ? 1.f : -1.f;
            ai.facing = static_cast<int>(dir);
            vel.x     = dir * speed;
            rb.body->SetLinearVelocity(vel);
        } else {
            // In attack range — start attack
            vel.x = 0.f;
            rb.body->SetLinearVelocity(vel);
            changeState(ai, bpd, EnemyState::Attack, bpd.currentPhase, entity, reg, clips);
            bpd.windowTimer = 0.f;
        }

        // If far — just return to patrol state label (same logic)
        if (dist > kVisionRange) {
            changeState(ai, bpd, EnemyState::Patrol, bpd.currentPhase, entity, reg, clips);
        }
        break;
    }

    // ── Attack window (windup → active → recovery) ────────────────────────────
    case EnemyState::Attack: {
        const auto* phaseData = table.phaseByFraction(1.f); // placeholder
        (void)phaseData;
        // Get current phase's windows
        const data::PhaseData* pd = nullptr;
        for (const auto& p : table.phases) {
            if (bpd.currentPhase == BossPhase::Stance   && p.name == "stance")    pd = &p;
            if (bpd.currentPhase == BossPhase::Enrage   && p.name == "enrage")    pd = &p;
            if (bpd.currentPhase == BossPhase::Desperate&& p.name == "desperate") pd = &p;
        }
        if (!pd || pd->windows.empty()) {
            changeState(ai, bpd, EnemyState::Chase, bpd.currentPhase, entity, reg, clips);
            break;
        }

        // Clamp window index
        bpd.attackWindowIdx = bpd.attackWindowIdx % static_cast<int>(pd->windows.size());
        const data::AttackWindow& w = pd->windows[bpd.attackWindowIdx];
        bpd.windowTimer += dt;

        const float windup   = w.windup;
        const float active   = windup + w.active;
        const float recovery = active + w.recovery;

        // Activate hitbox during active phase (uses existing Hitbox component)
        if (bpd.windowTimer >= windup && bpd.windowTimer < active) {
            if (reg.has<Hitbox>(entity)) {
                auto& hb = reg.get<Hitbox>(entity);
                hb.active = true;
                if (w.hitbox) {
                    hb.damage    = w.hitbox->damage;
                    hb.knockback = w.hitbox->knockback;
                }
                // Reposition sensor along boss facing direction
                if (hb.fixture) {
                    if (auto* poly = dynamic_cast<b2PolygonShape*>(hb.fixture->GetShape())) {
                        const float dir = static_cast<float>(ai.facing);
                        poly->SetAsBox(0.5f, 0.4f, b2Vec2{dir * 1.0f, 0.f}, 0.f);
                    }
                }
            }
            // Dash impulse (apply once at start of active)
            if (w.dashVel && !bpd.dashing) {
                bpd.dashing = true;
                const float dir = static_cast<float>(ai.facing);
                rb.body->SetLinearVelocity({dir * w.dashVel->x, w.dashVel->y});
            }
            // Radial fire (fire once during active)
            if (w.radial && !bpd.radialFired && spawnProjectile) {
                bpd.radialFired = true;
                const glm::vec3 bossPos3 = reg.has<Transform>(entity)
                    ? reg.get<Transform>(entity).position : glm::vec3{0.f};
                const glm::vec2 from{bossPos3.x, bossPos3.y};
                const int cnt = w.radial->count;
                for (int i = 0; i < cnt; ++i) {
                    const float angle = (2.f * 3.14159265f / cnt) * i;
                    const glm::vec2 dir = dirFromAngle(angle) * w.radial->speed;
                    spawnProjectile(from, dir, w.radial->damage);
                }
            }
        } else {
            // Deactivate hitbox outside active phase
            if (reg.has<Hitbox>(entity)) {
                auto& hb = reg.get<Hitbox>(entity);
                hb.active  = false;
                hb.hitMask = 0;
            }
        }

        // Halt horizontal during attack (except during dash)
        if (!bpd.dashing) {
            vel.x = 0.f;
            rb.body->SetLinearVelocity(vel);
        }

        // End of window: advance to next window or return to chase
        if (bpd.windowTimer >= recovery) {
            bpd.attackWindowIdx = (bpd.attackWindowIdx + 1)
                                   % static_cast<int>(pd->windows.size());
            bpd.dashing    = false;
            bpd.radialFired= false;

            // Alternate between attack windows; return to chase after full cycle
            if (bpd.attackWindowIdx == 0) {
                changeState(ai, bpd, EnemyState::Chase, bpd.currentPhase, entity, reg, clips);
                ai.attackCooldown = kAttackCooldown;
            } else {
                bpd.windowTimer = 0.f;
                setClip(entity, reg, bossClipName(bpd.currentPhase, EnemyState::Attack), clips);
            }
        }
        break;
    }

    case EnemyState::Hurt: {
        ai.stateTimer += dt;
        if (ai.stateTimer > 0.35f)
            changeState(ai, bpd, EnemyState::Chase, bpd.currentPhase, entity, reg, clips);
        break;
    }

    default:
        break;
    }

    // Sync facing to sprite flipX
    if (reg.has<Animator>(entity))
        reg.get<Animator>(entity).flipX = (ai.facing < 0);
}

} // namespace sys
