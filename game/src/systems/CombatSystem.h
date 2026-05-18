#pragma once
#include "engine/ecs/Registry.h"
#include "engine/physics/PhysicsWorld.h"
#include <box2d/box2d.h>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

namespace sys {

// Data attached to Box2D fixtures via GetUserData().pointer so the contact
// listener can identify which entity/component was touched.
enum class FixtureTag : uint8_t { None = 0, Hitbox, Hurtbox };
struct FixtureUserData {
    FixtureTag       tag{FixtureTag::None};
    eng::ecs::Entity entity;
};

struct HitEvent {
    eng::ecs::Entity attacker;
    eng::ecs::Entity victim;
    float            damage;
    float            knockbackX; // signed, already accounts for attacker facing
    float            knockbackY;
};

// Registers itself with PhysicsWorld and collects hit events to be resolved
// after physics.step().
class CombatContactListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override;
    void clearEvents() { m_events.clear(); }
    std::vector<HitEvent>& events() { return m_events; }

private:
    std::vector<HitEvent> m_events;
};

// Call before physics.step() to update hitbox active state.
void combatPreUpdate(eng::ecs::Registry& reg, float dt);

// Call after physics.step() to apply damage, knockback, flash, and invoke callbacks.
// onHitStop:      receives duration in seconds — set your scene's m_timeScale accordingly.
// onTrauma:       receives trauma amount [0..1] for the camera shake system.
// onHitSpark:     receives world-space hit position for particle spawn (optional).
// onDamageDealt:  receives world-space hit position + damage dealt (for HUD popups/SFX).
void combatPostUpdate(
    eng::ecs::Registry& reg,
    CombatContactListener& listener,
    const std::function<void(float)>& onHitStop,
    const std::function<void(float)>& onTrauma,
    const std::function<void(glm::vec3)>& onHitSpark = {},
    const std::function<void(glm::vec3, float)>& onDamageDealt = {});

} // namespace sys
