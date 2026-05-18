#include "systems/CombatSystem.h"
#include "components/Hitbox.h"
#include "components/Hurtbox.h"
#include "components/Health.h"
#include "components/PlayerControl.h"
#include "components/RigidBody.h"
#include "components/SpriteRenderer.h"
#include "engine/physics/PhysicsConstants.h"
#include <algorithm>
#include <cstdint>

namespace sys {

// Offset (in meters) of the hitbox sensor from body center along the attacker's X axis.
static constexpr float kHitboxOffsetX = 0.6f;
static constexpr float kHitboxHalfW   = 0.4f;
static constexpr float kHitboxHalfH   = 0.3f;

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

void combatPreUpdate(eng::ecs::Registry& reg, float dt) {
    // Flash timer → tint SpriteRenderer white on hit
    for (auto [e, hp, sr] : reg.view<Health, SpriteRenderer>()) {
        hp.invulnTimer = std::max(0.f, hp.invulnTimer - dt);
        hp.flashTimer  = std::max(0.f, hp.flashTimer  - dt);

        if (hp.flashTimer > 0.f) {
            const float t = hp.flashTimer / kFlashDuration;
            sr.tint = {1.f, 1.f - t * 0.6f, 1.f - t * 0.6f, 1.f}; // red flash
        } else {
            sr.tint = {1.f, 1.f, 1.f, hp.dead ? 0.3f : 1.f};
        }
    }

    // Manage hitbox activation based on attack timer and facing
    for (auto [e, ctrl, hb] : reg.view<PlayerControl, Hitbox>()) {
        if (!hb.fixture) continue;

        const bool inAttack = (ctrl.state == PlayerState::Attack);

        // The active frames are the middle 60% of the attack duration
        const float elapsed   = ctrl.attackDuration - ctrl.attackTimer;
        const float activeFrac = elapsed / ctrl.attackDuration;
        const bool  activeFrame = inAttack && activeFrac >= 0.2f && activeFrac <= 0.8f;

        if (activeFrame != hb.active) {
            hb.active = activeFrame;
            if (!activeFrame) hb.hitMask = 0; // reset hit targets on deactivation
        }

        // Reposition sensor along facing direction (Box2D sensor stays body-local)
        if (inAttack) {
            b2PolygonShape* shape =
                dynamic_cast<b2PolygonShape*>(hb.fixture->GetShape());
            if (shape) {
                b2Vec2 center{ctrl.facing * kHitboxOffsetX, 0.f};
                shape->SetAsBox(kHitboxHalfW, kHitboxHalfH, center, 0.f);
            }
        }
    }
}

// ------------------------------------------------------------------ post ----

void combatPostUpdate(
    eng::ecs::Registry& reg,
    CombatContactListener& listener,
    const std::function<void(float)>& onHitStop,
    const std::function<void(float)>& onTrauma)
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

            // Facing-aware knockback
            int facing = +1;
            if (reg.has<PlayerControl>(ev.attacker))
                facing = reg.get<PlayerControl>(ev.attacker).facing;
            ev.knockbackX = facing * hb.knockback.x;
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
    }

    listener.clearEvents();
}

} // namespace sys
