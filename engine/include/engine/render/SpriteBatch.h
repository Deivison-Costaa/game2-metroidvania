#pragma once
#include "engine/render/Camera.h"
#include "engine/render/Shader.h"
#include "engine/render/Texture.h"
#include <glm/glm.hpp>
#include <string_view>
#include <vector>

namespace eng::render {

// Batches up to kMaxSprites axis-aligned quads per flush, textured, in world-space.
// Usage:  batch.begin(camera); batch.draw(...); batch.end();
class SpriteBatch {
public:
    static constexpr int kMaxSprites = 1024;

    SpriteBatch(std::string_view vertPath, std::string_view fragPath);
    ~SpriteBatch();
    SpriteBatch(const SpriteBatch&)            = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    void begin(const Camera& camera);

    // Draw a world-space sprite quad with full texture.
    // pos: center in world, size: full width/height in world units, color: tint rgba.
    void draw(const Texture& tex,
              const glm::vec3& pos,
              const glm::vec2& size,
              const glm::vec4& color = glm::vec4(1.f),
              float            rotation = 0.f,
              bool             flipX = false);

    // Draw a world-space sprite quad with a UV sub-rectangle (for sprite sheets).
    // uvMin/uvMax are normalized [0..1] atlas coordinates.
    void draw(const Texture& tex,
              const glm::vec3& pos,
              const glm::vec2& size,
              const glm::vec2& uvMin,
              const glm::vec2& uvMax,
              const glm::vec4& color = glm::vec4(1.f),
              float            rotation = 0.f,
              bool             flipX = false);

    void end();

    // Exposes the underlying shader so callers can set per-frame uniforms
    // (e.g., fog density) before begin/end cycles. Uniforms persist until reset.
    Shader& shader() { return m_shader; }

private:
    struct Vertex {
        glm::vec3 pos;
        glm::vec2 uv;
        glm::vec4 color;
    };

    void flush();

    Shader               m_shader;
    unsigned int         m_vao{0};
    unsigned int         m_vbo{0};
    unsigned int         m_ibo{0};

    std::vector<Vertex>  m_vertices;
    const Texture*       m_currentTex{nullptr};
    int                  m_spriteCount{0};
    glm::mat4            m_viewProj{1.f};
};

} // namespace eng::render
