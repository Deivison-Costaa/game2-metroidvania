#pragma once
#include "engine/ecs/Entity.h"
#include <box2d/box2d.h>

struct Hurtbox {
    b2Fixture*        fixture{nullptr};
    eng::ecs::Entity  owner;          // entity that owns this hurtbox
};
