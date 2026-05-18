#include "systems/SpriteRenderSystem.h"
#include "components/Transform.h"
#include "components/SpriteRenderer.h"

namespace sys {

void spriteRenderUpdate(eng::ecs::Registry& reg, eng::render::SpriteBatch& batch) {
    for (auto [e, transform, sr] : reg.view<Transform, SpriteRenderer>()) {
        if (!sr.tex) continue;
        batch.draw(*sr.tex, transform.position, sr.size, sr.tint, transform.rotation);
    }
}

} // namespace sys
