#pragma once
#include "engine/animation/AnimationClip.h"
#include <memory>

struct Animator {
    std::shared_ptr<eng::animation::AnimationClip> clip;
    float time{0.f};
    int   frameIdx{0};
    bool  finished{false};  // true when a non-looping clip reaches its last frame
    bool  flipX{false};
};
