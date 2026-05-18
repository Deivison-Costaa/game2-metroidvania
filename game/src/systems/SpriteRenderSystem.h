#pragma once
#include "engine/ecs/Registry.h"
#include "engine/render/SpriteBatch.h"

namespace sys {

void spriteRenderUpdate(eng::ecs::Registry& reg, eng::render::SpriteBatch& batch);

} // namespace sys
