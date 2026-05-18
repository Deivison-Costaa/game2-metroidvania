#pragma once

namespace eng::render {

// Draws a fullscreen triangle using only gl_VertexID — no per-vertex attributes.
// Post-process shaders compute positions from gl_VertexID internally.
// Usage: quad.draw()  (bind shader + uniforms first)
class FullscreenQuad {
public:
    FullscreenQuad();
    ~FullscreenQuad();
    FullscreenQuad(const FullscreenQuad&)            = delete;
    FullscreenQuad& operator=(const FullscreenQuad&) = delete;

    void draw() const;

private:
    unsigned int m_vao{0};
};

} // namespace eng::render
