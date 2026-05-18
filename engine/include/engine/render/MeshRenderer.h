#pragma once
#include "engine/render/Camera.h"
#include "engine/render/Mesh.h"
#include "engine/render/Shader.h"
#include "engine/render/Texture.h"
#include <glm/glm.hpp>
#include <string_view>

namespace eng::render {

class MeshRenderer {
public:
    MeshRenderer(std::string_view vertPath, std::string_view fragPath);

    // Render a mesh with a given model transform.
    // whiteTex is used when tex == nullptr.
    void draw(const Mesh&    mesh,
              const Camera&  camera,
              const glm::mat4& model,
              const Texture* tex = nullptr);

    // Light controls (world-space direction, normalized)
    void setLight(const glm::vec3& dir,
                  const glm::vec3& color   = glm::vec3(1.f),
                  const glm::vec3& ambient = glm::vec3(0.15f));

private:
    Shader  m_shader;
    Texture m_white;

    glm::vec3 m_lightDir  { 0.6f,  1.f,  0.8f};
    glm::vec3 m_lightColor{1.f, 1.f, 1.f};
    glm::vec3 m_ambient   {0.15f, 0.15f, 0.2f};
};

} // namespace eng::render
