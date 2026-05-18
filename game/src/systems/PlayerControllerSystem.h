#pragma once
#include "engine/ecs/Registry.h"
#include "engine/input/InputManager.h"
#include "engine/physics/PhysicsWorld.h"

namespace sys {

void playerControllerUpdate(eng::ecs::Registry& reg,
                             const eng::input::InputManager& input,
                             eng::physics::PhysicsWorld& physics);

} // namespace sys
