#include "engine/render/MeshRenderer.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

namespace eng::render {

MeshRenderer::MeshRenderer(std::string_view vertPath, std::string_view fragPath)
    : m_shader(Shader::fromFiles(vertPath, fragPath))
    , m_white(Texture::fromWhite())
{}

void MeshRenderer::setLight(const glm::vec3& dir,
                             const glm::vec3& color,
                             const glm::vec3& ambient) {
    m_lightDir   = dir;
    m_lightColor = color;
    m_ambient    = ambient;
}

void MeshRenderer::draw(const Mesh& mesh,
                         const Camera& camera,
                         const glm::mat4& model,
                         const Texture* tex) {
    m_shader.bind();
    m_shader.set("uModel",      model);
    m_shader.set("uView",       camera.view());
    m_shader.set("uProj",       camera.projection());
    m_shader.set("uLightDir",   glm::normalize(m_lightDir));
    m_shader.set("uLightColor", m_lightColor);
    m_shader.set("uAmbient",    m_ambient);
    m_shader.set("uTexture",    0);

    const Texture& t = tex ? *tex : m_white;
    t.bind(0);

    mesh.draw();

    Texture::unbind(0);
    Shader::unbind();
}

} // namespace eng::render
