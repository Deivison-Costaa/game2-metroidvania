#include "systems/CombatSystem.h"
#include "components/Hitbox.h"
#include "components/Hurtbox.h"
#include "components/Health.h"
#include "components/PlayerControl.h"
#include "components/RigidBody.h"
#include "components/SpriteRenderer.h"
#include "components/EnemyAI.h"
#include "components/Transform.h"
#include "engine/physics/PhysicsConstants.h"
#include <algorithm>
#include <cstdint>

namespace sys {

// Seconds of invulnerability and flash after taking a hit
static constexpr float kInvulnDuration = 0.45f;
static constexpr float kFlashDuration  = 0.15f;

// ------------------------------------------------------------------ contact --

static FixtureUserData* userData(b2Fixture* f) {
    return reinterpret_cast<FixtureUserData*>(f->GetUserData().pointer);
}

void CombatContactListener::BeginContact(b2Contact* contact) {
    b2Fixture* fA = contact->GetFixtureA();
    b2Fixture* fB = contact->GetFixtureB();
    FixtureUserData* udA = userData(fA);
    FixtureUserData* udB = userData(fB);
    if (!udA || !udB) return;

    // Identify which fixture is hitbox and which is hurtbox
    FixtureUserData* hitUD  = nullptr;
    FixtureUserData* hurtUD = nullptr;
    b2Fixture*       hitFix = nullptr;

    if (udA->tag == FixtureTag::Hitbox && udB->tag == FixtureTag::Hurtbox) {
        hitUD = udA; hurtUD = udB; hitFix = fA;
    } else if (udB->tag == FixtureTag::Hitbox && udA->tag == FixtureTag::Hurtbox) {
        hitUD = udB; hurtUD = udA; hitFix = fB;
    }
    if (!hitUD || !hurtUD) return;

    // The hitbox fixture's body can be queried from the fixture
    (void)hitFix;
    m_events.push_back({hitUD->entity, hurtUD->entity, 0.f, 0.f, 0.f});
}

// ------------------------------------------------------------------ pre -----

void combatPreUpdate(eng::ecs::Registry& reg, float dt,
                     const eng::animation::AttackTable& attackTable) {
    // Flash timer → tint SpriteRenderer white on hit
    for (auto [e, hp, sr] : reg.view<Health, SpriteRenderer>()) {
        hp.invulnTimer = std::max(0.f, hp.invulnTimer - dt);
        hp.flashTimer  = std::max(0.f, hp.flashTimer  - dt);

        if (hp.flashTimer > 0.f) {
            const float t = hp.flashTimer / kFlashDuration;
            // HDR tint > 1 feeds the bloom pass for a bright damage flash
            sr.tint = {3.f * t + 1.f, 1.f - t * 0.8f, 1.f - t * 0.8f, 1.f};
        } else {
            sr.tint = {1.f, 1.f, 1.f, hp.dead ? 0.3f : 1.f};
        }
    }

    // Manage hitbox activation based on attack timer and facing (data from player_attacks.json)
    const eng::animation::AttackData* atk1 = attackTable.get("attack1");

    for (auto [e, ctrl, hb] : reg.view<PlayerControl, Hitbox>()) {
        if (!hb.fixture) continue;

        const bool  inAttack = (ctrl.state == PlayerState::Attack);
        const float elapsed  = ctrl.attackDuration - ctrl.attackTimer;

        // Determine active window from AttackTable; fall back to [20%-80%] of duration
        bool activeFrame = false;
        float offsetX = 0.6f, hw = 0.4f, hh = 0.3f;
        if (atk1 && !atk1->windows.empty()) {
            const auto& w = atk1->windows[0];
            activeFrame = inAttack && elapsed >= w.start && elapsed < w.end;
            offsetX = w.offset.x;
            hw      = w.halfSize.x;
            hh      = w.halfSize.y;
            if (activeFrame) {
                hb.damage    = w.damage;
                hb.knockback = w.knockback;
            }
        } else {
            const float frac = elapsed / ctrl.attackDuration;
            activeFrame = inAttack && frac >= 0.2f && frac <= 0.8f;
        }

        if (activeFrame != hb.active) {
            hb.active = activeFrame;
            if (!activeFrame) hb.hitMask = 0;
        }

        // Reposition sensor along facing direction every frame so it is always current
        if (auto* shape = dynamic_cast<b2PolygonShape*>(hb.fixture->GetShape())) {
            shape->SetAsBox(hw, hh, b2Vec2{ctrl.facing * offsetX, 0.f}, 0.f);
        }
    }
}

// ------------------------------------------------------------------ post ----

void combatPostUpdate(
    eng::ecs::Registry& reg,
    CombatContactListener& listener,
    const std::function<void(float)>& onHitStop,
    const std::function<void(float)>& onTrauma,
    const std::function<void(glm::vec3)>& onHitSpark,
    const std::function<void(glm::vec3, float)>& onDamageDealt)
{
    for (auto& ev : listener.events()) {
        // Validate both entities still exist
        if (!reg.valid(ev.attacker) || !reg.valid(ev.victim)) continue;

        // Check victim's hitMask — skip if already hit this swing
        if (reg.has<Hitbox>(ev.attacker)) {
            auto& hb = reg.get<Hitbox>(ev.attacker);
            if (!hb.active) continue;

            const uint64_t bit = 1ULL << (ev.victim.id & 63u);
            if (hb.hitMask & bit) continue;
            hb.hitMask |= bit;

            ev.damage = hb.damage;

            // Facing-aware knockback — resolve direction from attacker type
            float dirX = +1.f;
            if (reg.has<PlayerControl>(ev.attacker))
                dirX = float(reg.get<PlayerControl>(ev.attacker).facing);
            else if (reg.has<EnemyAI>(ev.attacker))
                dirX = float(reg.get<EnemyAI>(ev.attacker).facing);
            else if (reg.has<Transform>(ev.attacker) && reg.has<Transform>(ev.victim)) {
                const auto& aT = reg.get<Transform>(ev.attacker);
                const auto& vT = reg.get<Transform>(ev.victim);
                dirX = (vT.position.x >= aT.position.x) ? +1.f : -1.f;
            }
            ev.knockbackX = dirX * std::abs(hb.knockback.x);
            ev.knockbackY = hb.knockback.y;
        }

        if (!reg.has<Health>(ev.victim)) continue;
        auto& hp = reg.get<Health>(ev.victim);

        if (hp.invulnTimer > 0.f || hp.dead) continue;

        hp.current     -= ev.damage;
        hp.invulnTimer  = kInvulnDuration;
        hp.flashTimer   = kFlashDuration;
        if (hp.current <= 0.f) {
            hp.current = 0.f;
            hp.dead    = true;
        }

        // Knockback impulse on victim's body
        if (reg.has<RigidBody>(ev.victim)) {
            auto& rb = reg.get<RigidBody>(ev.victim);
            if (rb.body)
                rb.body->ApplyLinearImpulseToCenter(
                    {ev.knockbackX, ev.knockbackY}, true);
        }

        // Trigger game-feel callbacks
        if (onHitStop) onHitStop(0.06f);
        if (onTrauma)  onTrauma(0.65f);

        glm::vec3 victimPos{0.f};
        if (reg.has<Transform>(ev.victim))
            victimPos = reg.get<Transform>(ev.victim).position;

        if (onHitSpark)     onHitSpark(victimPos);
        if (onDamageDealt)  onDamageDealt(victimPos, ev.damage);
    }

    listener.clearEvents();
}

} // namespace sys
