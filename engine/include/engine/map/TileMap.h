#pragma once
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace eng::map {

struct Tileset {
    std::string image;  // absolute or relative path to the atlas PNG
    int firstGid   {1};
    int tileWidth  {32};
    int tileHeight {32};
    int columns    {1};
    int tileCount  {1};
};

struct TileLayer {
    std::string          name;
    int                  width  {0};
    int                  height {0};
    std::vector<uint32_t> gids; // row-major, 0 = empty tile
};

struct MapObject {
    int         id   {0};
    std::string name;
    std::string type;
    glm::vec2   pos  {0.f, 0.f}; // top-left in Tiled pixels (Y-down)
    glm::vec2   size {0.f, 0.f}; // pixels
    std::map<std::string, std::string> properties;
};

struct ObjectLayer {
    std::string           name;
    std::vector<MapObject> objects;
};

struct TileMap {
    int width      {0}; // map width in tiles
    int height     {0}; // map height in tiles
    int tileWidth  {32}; // pixels
    int tileHeight {32}; // pixels

    std::vector<Tileset>     tilesets;
    std::vector<TileLayer>   tileLayers;
    std::vector<ObjectLayer> objectLayers;

    // Map height in pixels (Y reference for coordinate conversion)
    int heightPx() const { return height * tileHeight; }

    // Resolve a GID to {tileset_index, local_tile_id}.
    // Returns {-1,-1} for GID=0 (empty) or unresolvable.
    std::pair<int,int> resolveTileset(uint32_t gid) const;

    // Convert Tiled pixel coordinates (Y-down, top-left of object) to
    // world-space center (Y-up, meters). pixelsPerMeter defaults to 32.
    glm::vec2 tiledToWorld(glm::vec2 tiledPos, glm::vec2 tiledSize,
                            float pixelsPerMeter = 32.f) const;

    static TileMap fromFile(std::string_view path);
};

} // namespace eng::map
