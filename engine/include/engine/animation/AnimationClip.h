#pragma once
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace eng::animation {

struct Frame {
    glm::vec2 uvMin{0.f, 0.f};
    glm::vec2 uvMax{1.f, 1.f};
    float     duration{0.1f}; // seconds
};

struct AnimationClip {
    std::string        name;
    std::vector<Frame> frames;
    bool               loop{true};

    // Load a clip from a JSON file. Throws std::runtime_error on failure.
    static AnimationClip fromFile(std::string_view path);
};

} // namespace eng::animation
