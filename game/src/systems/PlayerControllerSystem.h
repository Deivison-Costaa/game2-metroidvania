#pragma once
#include "engine/ecs/Registry.h"
#include "engine/input/InputManager.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/resources/ResourceManager.h"
#include "engine/animation/AnimationClip.h"
#include <functional>
#include <string>

namespace sys {

// onSfx: optional callback receiving SFX key ("jump", "land", "attack").
void playerControllerUpdate(
    eng::ecs::Registry& reg,
    const eng::input::InputManager& input,
    eng::physics::PhysicsWorld& physics,
    float dt,
    eng::resources::ResourceManager<eng::animation::AnimationClip>& clips,
    const std::function<void(const std::string&)>& onSfx = {});

} // namespace sys
