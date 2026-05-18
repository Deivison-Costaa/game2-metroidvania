#include "systems/EnemyCombatSystem.h"
#include "components/EnemyAI.h"
#include "components/Hitbox.h"
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

        // Reposition hitbox to face direction every frame — same technique as CombatSystem::combatPreUpdate
        if (hitbox.fixture) {
            const float facingSign = (ai.facing < 0) ? -1.f : 1.f;
            if (auto* poly = dynamic_cast<b2PolygonShape*>(hitbox.fixture->GetShape())) {
                poly->SetAsBox(0.35f, 0.25f, b2Vec2{facingSign * 0.55f, 0.f}, 0.f);
            }
        }
    }
}

} // namespace sys
