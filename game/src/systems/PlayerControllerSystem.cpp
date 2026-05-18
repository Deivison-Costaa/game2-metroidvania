#include "systems/PlayerControllerSystem.h"
#include "components/PlayerControl.h"
#include "components/RigidBody.h"
#include "engine/input/Action.h"

namespace sys {

void playerControllerUpdate(eng::ecs::Registry& reg,
                             const eng::input::InputManager& input,
                             eng::physics::PhysicsWorld& physics) {
    for (auto [e, ctrl, rb] : reg.view<PlayerControl, RigidBody>()) {
        if (!rb.body) continue;

        // Ground check via raycast
        ctrl.grounded = physics.isOnGround(rb.body, rb.halfH);

        // Horizontal movement — preserve current Y velocity
        const float horiz = input.axis(eng::input::Action::MoveLeft,
                                        eng::input::Action::MoveRight);
        b2Vec2 vel = rb.body->GetLinearVelocity();
        vel.x = horiz * ctrl.moveSpeed;
        rb.body->SetLinearVelocity(vel);

        // Jump — instant vertical velocity override (no accumulation)
        if (ctrl.grounded && input.pressed(eng::input::Action::Jump)) {
            vel = rb.body->GetLinearVelocity();
            vel.y = ctrl.jumpImpulse;
            rb.body->SetLinearVelocity(vel);
        }

        // Keep body awake while input is active
        if (horiz != 0.f) rb.body->SetAwake(true);
    }
}

} // namespace sys
