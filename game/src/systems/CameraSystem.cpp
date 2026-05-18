#include "systems/CameraSystem.h"
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <algorithm>

namespace sys {

// Simple xorshift RNG for shake noise — deterministic, zero allocation
static float shakeNoise(uint32_t& seed) {
    seed ^= seed << 13u;
    seed ^= seed >> 17u;
    seed ^= seed << 5u;
    // Map unsigned to [-1, 1]
    return static_cast<float>(static_cast<int32_t>(seed)) /
           static_cast<float>(0x7FFFFFFFu);
}

void cameraUpdate(CameraState& state,
                  const glm::vec3& playerPos,
                  int              playerFacing,
                  bool             inAttack,
                  float            realDt,
                  eng::render::Camera& cam)
{
    // --- Look-ahead target ---
    const float desiredX = playerPos.x + static_cast<float>(playerFacing) * state.lookAhead;

    // Deadzone: only move camTarget if desired is outside the dead band
    const float dx = desiredX - state.camTarget.x;
    if (std::abs(dx) > state.deadzoneHalf) {
        // Shift target so the deadzone edge just touches desiredX
        const float sign   = (dx > 0.f) ? 1.f : -1.f;
        state.camTarget.x += dx - sign * state.deadzoneHalf;
    }

    // Y follows player position with a slower lerp to feel less jittery
    const float desiredY = playerPos.y + state.camFixedY * 0.5f;
    state.camTarget.y   += (desiredY - state.camTarget.y) * 2.f * realDt;

    // Exponential lerp toward target
    state.pos += (state.camTarget - state.pos) * state.lerpK * realDt;

    // --- Dynamic zoom ---
    state.fovTarget = inAttack ? glm::radians(26.f) : glm::radians(30.f);
    state.fov      += (state.fovTarget - state.fov) * 4.f * realDt;
    cam.setFov(state.fov);

    // --- Screen shake (trauma model) ---
    state.trauma = std::max(0.f, state.trauma - 1.5f * realDt);
    const float shake  = state.trauma * state.trauma;
    const float shakeX = shakeNoise(state.shakeRng) * shake * 0.4f;
    const float shakeY = shakeNoise(state.shakeRng) * shake * 0.25f;

    // --- Write to camera ---
    const glm::vec3 finalPos{state.pos.x + shakeX, state.pos.y + shakeY, state.camZ};
    const glm::vec3 finalTgt{state.pos.x + shakeX, state.pos.y + shakeY, 0.f};

    cam.setPosition(finalPos);
    cam.setTarget(finalTgt);
    cam.setUp({0.f, 1.f, 0.f});
}

} // namespace sys
