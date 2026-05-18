#pragma once
#include "engine/render/Camera.h"
#include "engine/render/SpriteBatch.h"
#include "engine/render/Texture.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace eng::render {

struct ParallaxLayer {
    std::shared_ptr<Texture> tex;
    float factorX       {0.5f}; // [0=fixed, 1=scrolls with world] horizontal
    float yOffsetFromCam{0.f};  // layer center Y = camPos.y + yOffsetFromCam
    float worldH        {9.f};  // height in world units (should exceed screen height)
    float depthZ        {-2.f}; // draw depth (more negative = further back)
    float pixelsPerMeter{32.f};
};

// Draw all parallax layers using a single SpriteBatch.
// Each layer is drawn in its own begin()/end() block so the camera can differ.
// Layers are drawn in order (index 0 = furthest back).
// worldCam: the active world camera (used to compute apparent positions).
void drawParallax(SpriteBatch& batch,
                  const Camera& worldCam,
                  const std::vector<ParallaxLayer>& layers);

} // namespace eng::render
