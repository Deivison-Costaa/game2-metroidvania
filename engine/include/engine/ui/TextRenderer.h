#pragma once
#include "Font.h"
#include "engine/render/SpriteBatch.h"
#include <glm/glm.hpp>
#include <string>

namespace eng::ui {

// Renders text using a SpriteBatch (must be in begin()/end() scope already).
// screenPos is in the same space as the camera passed to batch.begin().
// For HUD text, use the UI orthographic camera (0..W, H..0 top-left origin).
void drawText(
    eng::render::SpriteBatch& batch,
    Font& font,
    const std::string& text,
    glm::vec2 screenPos,
    glm::vec4 color  = {1.f, 1.f, 1.f, 1.f},
    float     scale  = 1.f);

// Returns the pixel width of text rendered at scale=1.
float textWidth(const Font& font, const std::string& text);

} // namespace eng::ui
