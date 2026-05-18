#pragma once
#include "engine/render/Camera.h"
#include "engine/render/Shader.h"
#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <string_view>
#include <vector>

namespace eng::physics {

// Implements b2Draw to collect Box2D debug shapes and render them as GL_LINES.
class DebugDraw : public b2Draw {
public:
    DebugDraw(std::string_view vertPath, std::string_view fragPath);
    ~DebugDraw();

    DebugDraw(const DebugDraw&)            = delete;
    DebugDraw& operator=(const DebugDraw&) = delete;

    // Upload collected lines to GPU and draw. Call after b2World::DebugDraw().
    void render(const eng::render::Camera& camera);

    // b2Draw interface ───────────────────────────────────────────────────────
    void DrawPolygon(const b2Vec2* verts, int32 count, const b2Color& color) override;
    void DrawSolidPolygon(const b2Vec2* verts, int32 count, const b2Color& color) override;
    void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override;
    void DrawSolidCircle(const b2Vec2& center, float radius,
                         const b2Vec2& axis, const b2Color& color) override;
    void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;
    void DrawTransform(const b2Transform& xf) override;
    void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;

private:
    struct Vertex { glm::vec3 pos; glm::vec3 color; };

    void addLine(const b2Vec2& a, const b2Vec2& b, const b2Color& color);
    void circleLines(const b2Vec2& center, float radius, const b2Color& color,
                     int segments = 20);

    eng::render::Shader  m_shader;
    unsigned int         m_vao{0};
    unsigned int         m_vbo{0};
    std::vector<Vertex>  m_lines;
};

} // namespace eng::physics
