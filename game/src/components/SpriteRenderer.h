#pragma once
#include "engine/render/Texture.h"
#include <glm/glm.hpp>
#include <memory>

struct SpriteRenderer {
    std::shared_ptr<eng::render::Texture> tex;
    glm::vec2 size{1.f, 1.f};    // full width/height in world units (meters)
    glm::vec4 tint{1.f, 1.f, 1.f, 1.f};
    glm::vec2 uvMin{0.f, 0.f};   // atlas UV sub-rectangle
    glm::vec2 uvMax{1.f, 1.f};
    bool      flipX{false};
};
