#pragma once
#include <cstdint>

namespace eng::ecs {

struct Entity {
    uint32_t id  = 0;
    uint32_t gen = 0;
    bool operator==(const Entity&) const = default;
};

inline constexpr Entity kNullEntity{~0u, ~0u};

} // namespace eng::ecs
