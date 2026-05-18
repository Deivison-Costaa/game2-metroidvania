#pragma once
#include <glm/glm.hpp>

namespace eng::render {

class Camera {
public:
    static Camera perspective(float fovYRad, float aspect, float zNear, float zFar);
    static Camera orthographic(float left, float right, float bottom, float top,
                               float zNear, float zFar);

    // Position and orientation
    void setPosition(const glm::vec3& pos) { m_pos = pos; m_dirty = true; }
    void setTarget(const glm::vec3& target) { m_target = target; m_dirty = true; }
    void setUp(const glm::vec3& up)         { m_up = up;         m_dirty = true; }

    // Update aspect ratio (call from onEvent on window resize)
    void setAspect(float aspect);

    const glm::mat4& view()           const;
    const glm::mat4& projection()     const { return m_proj; }
    glm::mat4        viewProjection() const { return m_proj * view(); }

    const glm::vec3& position() const { return m_pos; }

private:
    Camera() = default;

    mutable glm::mat4 m_view {1.f};
    glm::mat4         m_proj {1.f};

    glm::vec3 m_pos   {0.f, 0.f,  5.f};
    glm::vec3 m_target{0.f, 0.f,  0.f};
    glm::vec3 m_up    {0.f, 1.f,  0.f};

    // Perspective params (stored for aspect-ratio updates)
    float m_fovY  {0.f};
    float m_aspect{1.f};
    float m_zNear {0.1f};
    float m_zFar  {100.f};
    bool  m_isPerspective{false};

    mutable bool m_dirty{true};
};

} // namespace eng::render
