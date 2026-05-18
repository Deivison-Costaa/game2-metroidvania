#include "engine/map/TileMapRenderer.h"

namespace eng::map {

void drawTileLayer(eng::render::SpriteBatch& batch,
                   const TileMap& map,
                   const TileLayer& layer,
                   const eng::render::Texture& tilesetTex,
                   const Tileset& ts,
                   float     pixelsPerMeter,
                   glm::vec2 mapOriginWorld,
                   float     depthZ)
{
    if (ts.columns <= 0 || layer.gids.empty()) return;

    const float tileW = static_cast<float>(ts.tileWidth)  / pixelsPerMeter;
    const float tileH = static_cast<float>(ts.tileHeight) / pixelsPerMeter;
    const float mapH  = static_cast<float>(map.height) * tileH;

    // UV dimensions of one tile in the atlas
    const float texW    = static_cast<float>(tilesetTex.width());
    const float texH    = static_cast<float>(tilesetTex.height());
    const float uvTileW = static_cast<float>(ts.tileWidth)  / texW;
    const float uvTileH = static_cast<float>(ts.tileHeight) / texH;

    for (int row = 0; row < layer.height; ++row) {
        for (int col = 0; col < layer.width; ++col) {
            const int gidIdx = row * layer.width + col;
            if (gidIdx >= static_cast<int>(layer.gids.size())) continue;

            const uint32_t gid = layer.gids[gidIdx];
            if (gid == 0) continue;

            auto [tsIdx, localId] = map.resolveTileset(gid);
            if (tsIdx < 0) continue;
            // Skip tiles belonging to a different tileset
            if (map.tilesets[tsIdx].firstGid != ts.firstGid) continue;

            // Atlas UV (row-major layout in the PNG)
            const int ac  = localId % ts.columns;
            const int ar  = localId / ts.columns;
            const glm::vec2 uvMin{ac       * uvTileW,  ar       * uvTileH};
            const glm::vec2 uvMax{(ac + 1) * uvTileW, (ar + 1) * uvTileH};

            // World center: Tiled row 0 is the TOP of the map, Y-down.
            // Convert to game-world Y-up by flipping relative to map height.
            const float wx = mapOriginWorld.x + (static_cast<float>(col) + 0.5f) * tileW;
            const float wy = mapOriginWorld.y + mapH - (static_cast<float>(row) + 0.5f) * tileH;

            batch.draw(tilesetTex,
                       glm::vec3{wx, wy, depthZ},
                       glm::vec2{tileW, tileH},
                       uvMin, uvMax);
        }
    }
}

} // namespace eng::map
