#include "systems/AnimationSystem.h"
#include "components/Animator.h"
#include "components/SpriteRenderer.h"

namespace sys {

void animationUpdate(eng::ecs::Registry& reg, float dt) {
    for (auto [e, anim, sr] : reg.view<Animator, SpriteRenderer>()) {
        if (!anim.clip || anim.clip->frames.empty()) continue;

        const int frameCount = static_cast<int>(anim.clip->frames.size());
        if (anim.frameIdx >= frameCount) anim.frameIdx = frameCount - 1;

        anim.time += dt;
        while (anim.time >= anim.clip->frames[static_cast<std::size_t>(anim.frameIdx)].duration) {
            anim.time -= anim.clip->frames[static_cast<std::size_t>(anim.frameIdx)].duration;
            if (anim.frameIdx + 1 < frameCount) {
                ++anim.frameIdx;
            } else if (anim.clip->loop) {
                anim.frameIdx = 0;
            } else {
                anim.finished = true;
                break;
            }
        }

        const auto& frame = anim.clip->frames[static_cast<std::size_t>(anim.frameIdx)];
        sr.uvMin  = frame.uvMin;
        sr.uvMax  = frame.uvMax;
        sr.flipX  = anim.flipX;
    }
}

} // namespace sys
