#pragma once
#include "engine/ecs/Entity.h"
#include <glm/glm.hpp>
#include <box2d/box2d.h>
#include <cstdint>

struct Hitbox {
    b2Fixture* fixture{nullptr};    // sensor attached to attacker body
    float      damage{1.f};
    glm::vec2  knockback{4.f, 1.5f}; // m/s impulse; X is mirrored by attacker facing
    bool       active{false};       // enabled for the active frames of an attack
    uint64_t   hitMask{0};          // bitmask of entity IDs already hit this swing
};
