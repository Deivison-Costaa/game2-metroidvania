#include "systems/PlayerControllerSystem.h"
#include "components/PlayerControl.h"
#include "components/RigidBody.h"
#include "components/Animator.h"
#include "engine/input/Action.h"
#include <algorithm>
#include <cmath>

namespace sys {

// Maps a PlayerState to the animation clip key registered in the ResourceManager.
static const char* clipName(PlayerState s) {
    switch (s) {
        case PlayerState::Run:    return "run";
        case PlayerState::Jump:   return "jump";
        case PlayerState::Fall:   return "fall";
        case PlayerState::Attack: return "attack";
        case PlayerState::Hurt:   return "hurt";
        default:                  return "idle";
    }
}

void playerControllerUpdate(
    eng::ecs::Registry& reg,
    const eng::input::InputManager& input,
    eng::physics::PhysicsWorld& physics,
    float dt,
    eng::resources::ResourceManager<eng::animation::AnimationClip>& clips)
{
    for (auto [e, ctrl, rb] : reg.view<PlayerControl, RigidBody>()) {
        if (!rb.body) continue;

        // --- Timers ---
        ctrl.stateTime       += dt;
        ctrl.coyoteTimer      = std::max(0.f, ctrl.coyoteTimer      - dt);
        ctrl.jumpBufferTimer  = std::max(0.f, ctrl.jumpBufferTimer  - dt);
        ctrl.attackTimer      = std::max(0.f, ctrl.attackTimer      - dt);

        // --- Ground check ---
        const bool prevGrounded = ctrl.wasGrounded;
        ctrl.grounded    = physics.isOnGround(rb.body, rb.halfH);
        ctrl.wasGrounded = ctrl.grounded;

        // Coyote time: grant a jump window just after walking off an edge
        if (prevGrounded && !ctrl.grounded && ctrl.state != PlayerState::Jump)
            ctrl.coyoteTimer = ctrl.coyoteTime;

        // --- Input ---
        const float horiz = input.axis(eng::input::Action::MoveLeft,
                                        eng::input::Action::MoveRight);

        // Jump buffer: record a jump press before landing
        if (input.pressed(eng::input::Action::Jump))
            ctrl.jumpBufferTimer = ctrl.jumpBufferTime;

        b2Vec2 vel = rb.body->GetLinearVelocity();

        // --- Apply jump ---
        if (ctrl.attackTimer <= 0.f) { // no jumping mid-attack
            const bool canJump = ctrl.grounded || ctrl.coyoteTimer > 0.f;
            if (ctrl.jumpBufferTimer > 0.f && canJump) {
                vel.y = ctrl.jumpImpulse;
                rb.body->SetLinearVelocity(vel);
                ctrl.jumpBufferTimer = 0.f;
                ctrl.coyoteTimer     = 0.f;
            }
        }

        // Variable jump height: cut ascent on early release
        if (input.released(eng::input::Action::Jump) && vel.y > 0.f)
            vel.y *= 0.5f;

        // --- Horizontal movement ---
        if (ctrl.attackTimer > 0.f) {
            // Lock horizontal motion during attack
            vel.x = 0.f;
        } else {
            vel.x = horiz * ctrl.moveSpeed;
            if (std::abs(horiz) > 0.01f)
                ctrl.facing = (horiz > 0.f) ? +1 : -1;
        }
        rb.body->SetLinearVelocity(vel);

        if (horiz != 0.f) rb.body->SetAwake(true);

        // --- Attack input ---
        if (input.pressed(eng::input::Action::Attack) && ctrl.attackTimer <= 0.f) {
            ctrl.attackTimer = ctrl.attackDuration;
        }

        // --- State transition ---
        PlayerState newState;
        if (ctrl.attackTimer > 0.f) {
            newState = PlayerState::Attack;
        } else if (!ctrl.grounded && vel.y > 0.1f) {
            newState = PlayerState::Jump;
        } else if (!ctrl.grounded && vel.y <= 0.1f) {
            newState = PlayerState::Fall;
        } else if (std::abs(horiz) > 0.01f) {
            newState = PlayerState::Run;
        } else {
            newState = PlayerState::Idle;
        }

        // On state change: swap animation clip and reset animator
        if (newState != ctrl.state) {
            ctrl.state     = newState;
            ctrl.stateTime = 0.f;

            const char* key = clipName(newState);
            if (clips.has(key)) {
                if (reg.has<Animator>(e)) {
                    auto& anim       = reg.get<Animator>(e);
                    anim.clip        = clips.get(key);
                    anim.time        = 0.f;
                    anim.frameIdx    = 0;
                    anim.finished    = false;
                }
            }
        }

        // Sync flip direction every frame (facing can change without state change)
        if (reg.has<Animator>(e))
            reg.get<Animator>(e).flipX = (ctrl.facing < 0);
    }
}

} // namespace sys
