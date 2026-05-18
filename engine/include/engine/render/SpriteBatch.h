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

    // Draw a world-space sprite quad.
    // pos: center in world, size: half-extents in world units, color: tint rgba.
    void draw(const Texture& tex,
              const glm::vec3& pos,
              const glm::vec2& size,
              const glm::vec4& color = glm::vec4(1.f),
              float            rotation = 0.f);

    void end();

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
