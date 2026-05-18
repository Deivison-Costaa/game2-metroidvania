#pragma once
#include "engine/map/TileMap.h"
#include "engine/render/SpriteBatch.h"
#include "engine/render/Texture.h"
#include <glm/glm.hpp>

namespace eng::map {

// Draw one tile layer via an already-begun SpriteBatch.
// Only tiles belonging to 'ts' are drawn; others are silently skipped.
// pixelsPerMeter: scale factor (default 32 — 1 tile = 1m).
// mapOriginWorld: world-space offset of the map's bottom-left corner.
void drawTileLayer(eng::render::SpriteBatch& batch,
                   const TileMap& map,
                   const TileLayer& layer,
                   const eng::render::Texture& tilesetTex,
                   const Tileset& ts,
                   float     pixelsPerMeter  = 32.f,
                   glm::vec2 mapOriginWorld  = {0.f, 0.f},
                   float     depthZ          = -0.1f);

} // namespace eng::map
