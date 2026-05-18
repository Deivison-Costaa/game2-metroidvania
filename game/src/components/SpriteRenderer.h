#pragma once
#include "engine/render/Texture.h"
#include <glm/glm.hpp>
#include <memory>

struct SpriteRenderer {
    std::shared_ptr<eng::render::Texture> tex;
    glm::vec2 size{0.5f, 0.5f};  // half-extents in world units (meters)
    glm::vec4 tint{1.f, 1.f, 1.f, 1.f};
};
