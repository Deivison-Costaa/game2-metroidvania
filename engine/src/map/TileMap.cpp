#include "engine/map/TileMap.h"
#include "engine/core/Log.h"
#include <pugixml.hpp>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace eng::map {

// Parse comma-separated GID values from a Tiled CSV data block.
static std::vector<uint32_t> parseCSV(const char* data) {
    std::vector<uint32_t> result;
    result.reserve(512);
    std::istringstream ss(data);
    std::string token;
    while (std::getline(ss, token, ',')) {
        const char* p = token.c_str();
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') ++p;
        if (*p) result.push_back(static_cast<uint32_t>(std::strtoul(p, nullptr, 10)));
    }
    return result;
}

std::pair<int,int> TileMap::resolveTileset(uint32_t gid) const {
    if (gid == 0) return {-1, -1};
    int best = -1;
    for (int i = 0; i < static_cast<int>(tilesets.size()); ++i) {
        if (tilesets[i].firstGid <= static_cast<int>(gid)) best = i;
    }
    if (best < 0) return {-1, -1};
    return {best, static_cast<int>(gid) - tilesets[best].firstGid};
}

glm::vec2 TileMap::tiledToWorld(glm::vec2 tiledPos, glm::vec2 tiledSize,
                                 float pixelsPerMeter) const {
    // Tiled: origin top-left, Y-down.
    // World: origin bottom-left of map, Y-up.
    const float mapHPx = static_cast<float>(heightPx());
    const float cx = (tiledPos.x + tiledSize.x * 0.5f) / pixelsPerMeter;
    const float cy = (mapHPx - tiledPos.y - tiledSize.y * 0.5f) / pixelsPerMeter;
    return {cx, cy};
}

TileMap TileMap::fromFile(std::string_view path) {
    pugi::xml_document doc;
    const pugi::xml_parse_result res = doc.load_file(std::string(path).c_str());
    if (!res)
        throw std::runtime_error(std::string("TileMap: cannot parse '") +
                                 std::string(path) + "': " + res.description());

    const auto mapNode = doc.child("map");
    if (!mapNode)
        throw std::runtime_error(std::string("TileMap: no <map> root in '") +
                                 std::string(path) + "'");

    const std::filesystem::path mapDir =
        std::filesystem::path(path).parent_path();

    TileMap tm;
    tm.width      = mapNode.attribute("width").as_int();
    tm.height     = mapNode.attribute("height").as_int();
    tm.tileWidth  = mapNode.attribute("tilewidth").as_int();
    tm.tileHeight = mapNode.attribute("tileheight").as_int();

    // ── Tilesets ────────────────────────────────────────────────────────────
    for (const auto tsNode : mapNode.children("tileset")) {
        Tileset ts;
        ts.firstGid   = tsNode.attribute("firstgid").as_int();
        ts.tileWidth  = tsNode.attribute("tilewidth").as_int(tm.tileWidth);
        ts.tileHeight = tsNode.attribute("tileheight").as_int(tm.tileHeight);
        ts.columns    = tsNode.attribute("columns").as_int();
        ts.tileCount  = tsNode.attribute("tilecount").as_int();

        const char* extSrc = tsNode.attribute("source").value();
        if (extSrc && *extSrc) {
            // External .tsx — reload the tileset XML
            pugi::xml_document tsx;
            const auto tsxPath = mapDir / extSrc;
            if (tsx.load_file(tsxPath.c_str())) {
                const auto root = tsx.child("tileset");
                ts.tileWidth  = root.attribute("tilewidth").as_int(ts.tileWidth);
                ts.tileHeight = root.attribute("tileheight").as_int(ts.tileHeight);
                ts.columns    = root.attribute("columns").as_int(ts.columns);
                ts.tileCount  = root.attribute("tilecount").as_int(ts.tileCount);
                const auto img = root.child("image");
                if (img)
                    ts.image = (mapDir / img.attribute("source").value()).string();
            }
        } else {
            const auto img = tsNode.child("image");
            if (img)
                ts.image = (mapDir / img.attribute("source").value()).string();
        }
        tm.tilesets.push_back(std::move(ts));
    }

    // ── Tile layers ─────────────────────────────────────────────────────────
    for (const auto layerNode : mapNode.children("layer")) {
        TileLayer tl;
        tl.name   = layerNode.attribute("name").value();
        tl.width  = layerNode.attribute("width").as_int(tm.width);
        tl.height = layerNode.attribute("height").as_int(tm.height);

        const auto dataNode = layerNode.child("data");
        const std::string enc = dataNode.attribute("encoding").value();
        if (enc == "csv") {
            tl.gids = parseCSV(dataNode.text().get());
        } else {
            LOG_WARN("TileMap: unsupported encoding '{}' in layer '{}' — skipped",
                     enc, tl.name);
        }
        tm.tileLayers.push_back(std::move(tl));
    }

    // ── Object layers ───────────────────────────────────────────────────────
    for (const auto grpNode : mapNode.children("objectgroup")) {
        ObjectLayer ol;
        ol.name = grpNode.attribute("name").value();
        for (const auto objNode : grpNode.children("object")) {
            MapObject obj;
            obj.id   = objNode.attribute("id").as_int();
            obj.name = objNode.attribute("name").value();
            obj.type = objNode.attribute("type").value();
            obj.pos  = {objNode.attribute("x").as_float(),
                        objNode.attribute("y").as_float()};
            obj.size = {objNode.attribute("width").as_float(),
                        objNode.attribute("height").as_float()};
            for (const auto prop :
                 objNode.child("properties").children("property")) {
                obj.properties[prop.attribute("name").value()] =
                    prop.attribute("value").value();
            }
            ol.objects.push_back(std::move(obj));
        }
        tm.objectLayers.push_back(std::move(ol));
    }

    LOG_INFO("TileMap: loaded '{}' [{}×{} tiles | {} tile layers | {} object layers]",
             std::string(path), tm.width, tm.height,
             tm.tileLayers.size(), tm.objectLayers.size());

    return tm;
}

} // namespace eng::map
