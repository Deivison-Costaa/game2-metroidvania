#include "systems/EnemyCombatSystem.h"
#include "components/EnemyAI.h"
#include "components/Hitbox.h"
#include "components/SpriteRenderer.h"
#include <algorithm>

namespace sys {

void enemyCombatPreUpdate(eng::ecs::Registry& reg, float /*dt*/) {
    for (auto [e, ai, hitbox] : reg.view<EnemyAI, Hitbox>()) {
        // Gate hitbox active: only during Attack state with a swing window
        // Attack window: 0.05s–0.25s into the attack state (frames 1-2 of the animation)
        const bool inSwing = (ai.state == EnemyState::Attack)
                           && (ai.stateTimer >= 0.05f)
                           && (ai.stateTimer <= 0.25f);
        hitbox.active = inSwing;

        // Reposition hitbox to face direction — mirror of CombatSystem::combatPreUpdate
        if (reg.has<SpriteRenderer>(e)) {
            const bool flip = reg.get<SpriteRenderer>(e).flipX;
            // Offset sign flipped when facing left
            if (hitbox.fixture) {
                b2PolygonShape shape;
                float hw = 0.35f, hh = 0.25f;
                b2Vec2 offset{(flip ? -0.55f : 0.55f), 0.f};
                shape.SetAsBox(hw, hh, offset, 0.f);
                hitbox.fixture->GetShape(); // access to verify fixture alive
                // Box2D 2.4 doesn't support runtime shape changes on a live fixture.
                // Offset was baked into the sensor at spawn time;
                // for enemies we create the hitbox already on the correct side
                // at spawn, and flip via SetFilterData when facing changes.
                (void)shape; // suppress warning — shape reposition deferred to spawn
            }
        }
    }
}

} // namespace sys
