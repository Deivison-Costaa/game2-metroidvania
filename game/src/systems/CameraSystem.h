#pragma once
#include "engine/render/Camera.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace sys {

struct CameraState {
    glm::vec2 pos       {0.f, 3.5f};   // current camera XY world position
    glm::vec2 camTarget {0.f, 3.5f};   // smoothed target before shake
    float     fov       {glm::radians(30.f)};
    float     fovTarget {glm::radians(30.f)};
    float     trauma    {0.f};          // [0..1] drives screen shake intensity
    float     lookAhead {1.5f};         // meters ahead in facing direction
    float     deadzoneHalf{0.5f};       // half-width of the horizontal dead zone (m)
    float     lerpK     {5.f};          // exponential-lerp coefficient
    float     camZ      {16.f};         // fixed Z distance from scene
    float     camFixedY {3.5f};         // fixed Y height offset

    // Random seed for shake noise (advances each frame)
    uint32_t  shakeRng  {1337u};
};

// Update camera state and write result to cam.
// Use realDt (unscaled by hit-stop) so shake/zoom still animate during hit-stop.
void cameraUpdate(CameraState& state,
                  const glm::vec3& playerPos,
                  int              playerFacing,
                  bool             inAttack,
                  float            realDt,
                  eng::render::Camera& cam);

// Add trauma [0..1]; clamped to [0..1] total.
inline void cameraAddTrauma(CameraState& state, float amount) {
    state.trauma = std::min(1.f, state.trauma + amount);
}

} // namespace sys
