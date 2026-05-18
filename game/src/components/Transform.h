#pragma once
#include <glm/glm.hpp>

struct Transform {
    glm::vec3 position{0.f};
    float     rotation{0.f};  // radians
    glm::vec2 scale{1.f, 1.f};
};
