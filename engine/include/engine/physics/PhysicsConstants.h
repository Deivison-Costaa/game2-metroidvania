#pragma once
#include <box2d/box2d.h>
#include <glm/glm.hpp>

namespace eng::physics {

// World unit convention: 1 meter = 32 pixels
inline constexpr float kPixelsPerMeter = 32.f;

inline constexpr float toMeters(float px)  noexcept { return px / kPixelsPerMeter; }
inline constexpr float toPixels(float m)   noexcept { return m  * kPixelsPerMeter; }

inline b2Vec2 toB2(const glm::vec2& v) noexcept { return {v.x, v.y}; }
inline glm::vec2 toGlm(const b2Vec2& v) noexcept { return {v.x, v.y}; }

} // namespace eng::physics
