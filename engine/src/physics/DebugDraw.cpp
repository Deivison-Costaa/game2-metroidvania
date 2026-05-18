#include "engine/physics/DebugDraw.h"
#include "engine/physics/PhysicsConstants.h"
#include <GL/glew.h>
#include <cmath>
#include <numbers>

namespace eng::physics {

DebugDraw::DebugDraw(std::string_view vertPath, std::string_view fragPath)
    : m_shader(eng::render::Shader::fromFiles(vertPath, fragPath))
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // pos (vec3) at location 0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, pos)));
    // color (vec3) at location 1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, color)));

    glBindVertexArray(0);

    SetFlags(e_shapeBit | e_jointBit);
}

DebugDraw::~DebugDraw() {
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void DebugDraw::render(const eng::render::Camera& camera) {
    if (m_lines.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_lines.size() * sizeof(Vertex)),
                 m_lines.data(), GL_STREAM_DRAW);

    m_shader.bind();
    m_shader.set("uViewProj", camera.viewProjection());

    glBindVertexArray(m_vao);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_lines.size()));
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);

    m_lines.clear();
}

// ── b2Draw interface ──────────────────────────────────────────────────────────

void DebugDraw::DrawPolygon(const b2Vec2* verts, int32 count, const b2Color& c) {
    for (int32 i = 0; i < count; ++i)
        addLine(verts[i], verts[(i + 1) % count], c);
}

void DebugDraw::DrawSolidPolygon(const b2Vec2* verts, int32 count, const b2Color& c) {
    b2Color dim{c.r * 0.5f, c.g * 0.5f, c.b * 0.5f, c.a};
    DrawPolygon(verts, count, dim);
}

void DebugDraw::DrawCircle(const b2Vec2& center, float radius, const b2Color& c) {
    circleLines(center, radius, c);
}

void DebugDraw::DrawSolidCircle(const b2Vec2& center, float radius,
                                 const b2Vec2& axis, const b2Color& c) {
    circleLines(center, radius, c);
    addLine(center, {center.x + axis.x * radius, center.y + axis.y * radius}, c);
}

void DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& c) {
    addLine(p1, p2, c);
}

void DebugDraw::DrawTransform(const b2Transform& xf) {
    constexpr float kAxisLen = 0.5f;
    const b2Vec2 o = xf.p;
    const b2Vec2 x = {o.x + xf.q.GetXAxis().x * kAxisLen,
                       o.y + xf.q.GetXAxis().y * kAxisLen};
    const b2Vec2 y = {o.x + xf.q.GetYAxis().x * kAxisLen,
                       o.y + xf.q.GetYAxis().y * kAxisLen};
    addLine(o, x, {1, 0, 0, 1});
    addLine(o, y, {0, 1, 0, 1});
}

void DebugDraw::DrawPoint(const b2Vec2& p, float /*size*/, const b2Color& c) {
    constexpr float kHalf = 0.05f;
    addLine({p.x - kHalf, p.y}, {p.x + kHalf, p.y}, c);
    addLine({p.x, p.y - kHalf}, {p.x, p.y + kHalf}, c);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void DebugDraw::addLine(const b2Vec2& a, const b2Vec2& b, const b2Color& c) {
    const glm::vec3 col{c.r, c.g, c.b};
    // Box2D positions are in meters; the camera sees world units (meters directly)
    m_lines.push_back({{a.x, a.y, 0.f}, col});
    m_lines.push_back({{b.x, b.y, 0.f}, col});
}

void DebugDraw::circleLines(const b2Vec2& center, float radius,
                             const b2Color& c, int segments) {
    const float step = 2.f * std::numbers::pi_v<float> / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i) {
        const float a0 = static_cast<float>(i)     * step;
        const float a1 = static_cast<float>(i + 1) * step;
        b2Vec2 p0{center.x + radius * std::cos(a0), center.y + radius * std::sin(a0)};
        b2Vec2 p1{center.x + radius * std::cos(a1), center.y + radius * std::sin(a1)};
        addLine(p0, p1, c);
    }
}

} // namespace eng::physics
