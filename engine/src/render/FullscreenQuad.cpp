#include "engine/render/FullscreenQuad.h"
#include <GL/glew.h>

namespace eng::render {

FullscreenQuad::FullscreenQuad() {
    glGenVertexArrays(1, &m_vao);
}

FullscreenQuad::~FullscreenQuad() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void FullscreenQuad::draw() const {
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

} // namespace eng::render
