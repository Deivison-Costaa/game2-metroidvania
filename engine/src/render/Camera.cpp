#include "engine/render/Camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace eng::render {

Camera Camera::perspective(float fovYRad, float aspect, float zNear, float zFar) {
    Camera c;
    c.m_isPerspective = true;
    c.m_fovY          = fovYRad;
    c.m_aspect        = aspect;
    c.m_zNear         = zNear;
    c.m_zFar          = zFar;
    c.m_proj          = glm::perspective(fovYRad, aspect, zNear, zFar);
    return c;
}

Camera Camera::orthographic(float left, float right, float bottom, float top,
                            float zNear, float zFar) {
    Camera c;
    c.m_isPerspective = false;
    c.m_proj          = glm::ortho(left, right, bottom, top, zNear, zFar);
    return c;
}

void Camera::setAspect(float aspect) {
    if (!m_isPerspective) return;
    m_aspect = aspect;
    m_proj   = glm::perspective(m_fovY, m_aspect, m_zNear, m_zFar);
}

void Camera::setFov(float fovYRad) {
    if (!m_isPerspective) return;
    m_fovY = fovYRad;
    m_proj = glm::perspective(m_fovY, m_aspect, m_zNear, m_zFar);
}

const glm::mat4& Camera::view() const {
    if (m_dirty) {
        m_view  = glm::lookAt(m_pos, m_target, m_up);
        m_dirty = false;
    }
    return m_view;
}

} // namespace eng::render
