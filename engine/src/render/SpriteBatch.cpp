#include "engine/render/SpriteBatch.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <stdexcept>

namespace eng::render {

SpriteBatch::SpriteBatch(std::string_view vertPath, std::string_view fragPath)
    : m_shader(Shader::fromFiles(vertPath, fragPath))
{
    m_vertices.reserve(static_cast<std::size_t>(kMaxSprites) * 4);

    // Pre-build index buffer: each quad = 2 triangles = 6 indices
    std::vector<unsigned int> indices;
    indices.reserve(static_cast<std::size_t>(kMaxSprites) * 6);
    for (int i = 0; i < kMaxSprites; ++i) {
        const unsigned int base = static_cast<unsigned int>(i) * 4u;
        indices.insert(indices.end(), {
            base+0, base+1, base+2,
            base+0, base+2, base+3
        });
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);

    // Index buffer (static — never changes)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(), GL_STATIC_DRAW);

    // Vertex buffer (stream — re-uploaded each flush)
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(kMaxSprites) * 4 * static_cast<GLsizeiptr>(sizeof(Vertex)),
        nullptr, GL_STREAM_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));
    // aPos (location 0): vec3
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(Vertex, pos)));
    // aUV  (location 1): vec2
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(Vertex, uv)));
    // aColor (location 2): vec4
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(Vertex, color)));

    glBindVertexArray(0);
}

SpriteBatch::~SpriteBatch() {
    glDeleteBuffers(1, &m_ibo);
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
}

void SpriteBatch::begin(const Camera& camera) {
    m_viewProj    = camera.viewProjection();
    m_spriteCount = 0;
    m_currentTex  = nullptr;
    m_vertices.clear();
}

void SpriteBatch::draw(const Texture& tex,
                       const glm::vec3& pos,
                       const glm::vec2& size,
                       const glm::vec4& color,
                       float rotation) {
    // Flush if switching textures or batch is full
    if (m_currentTex && m_currentTex->id() != tex.id())
        flush();
    if (m_spriteCount >= kMaxSprites)
        flush();

    m_currentTex = &tex;

    // Build quad corners in local space then rotate around Z
    const float hw = size.x * 0.5f;
    const float hh = size.y * 0.5f;

    std::array<glm::vec2, 4> corners = {{
        {-hw, -hh}, { hw, -hh}, { hw,  hh}, {-hw,  hh}
    }};

    const float c = glm::cos(rotation);
    const float s = glm::sin(rotation);

    const std::array<glm::vec2, 4> uvs = {{
        {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}
    }};

    for (int i = 0; i < 4; ++i) {
        const float rx = corners[i].x * c - corners[i].y * s;
        const float ry = corners[i].x * s + corners[i].y * c;
        m_vertices.push_back({
            glm::vec3(pos.x + rx, pos.y + ry, pos.z),
            uvs[static_cast<std::size_t>(i)],
            color
        });
    }
    ++m_spriteCount;
}

void SpriteBatch::end() {
    flush();
}

void SpriteBatch::flush() {
    if (m_vertices.empty() || !m_currentTex) return;

    // Disable depth writes so transparent sprites don't occlude each other
    glDepthMask(GL_FALSE);

    m_shader.bind();
    m_shader.set("uViewProj", m_viewProj);
    m_shader.set("uTexture",  0);
    m_currentTex->bind(0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // Orphan-and-fill for efficient stream upload
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(kMaxSprites) * 4 * static_cast<GLsizeiptr>(sizeof(Vertex)),
        nullptr, GL_STREAM_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>(m_vertices.size() * sizeof(Vertex)),
        m_vertices.data());

    glDrawElements(GL_TRIANGLES,
        m_spriteCount * 6, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
    Shader::unbind();
    Texture::unbind(0);

    glDepthMask(GL_TRUE);

    m_vertices.clear();
    m_spriteCount = 0;
    m_currentTex  = nullptr;
}

} // namespace eng::render
