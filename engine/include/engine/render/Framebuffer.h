#pragma once
#include <GL/glew.h>

namespace eng::render {

// RAII wrapper for an OpenGL FBO with one color attachment + optional depth renderbuffer.
// Move-only. Resize re-allocates GPU storage.
class Framebuffer {
public:
    Framebuffer(int w, int h, GLenum colorFormat, bool depth);
    ~Framebuffer();
    Framebuffer(Framebuffer&&) noexcept;
    Framebuffer& operator=(Framebuffer&&) noexcept;
    Framebuffer(const Framebuffer&)            = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void bind()   const;
    void unbind() const;
    void resize(int w, int h);

    unsigned int colorTexture() const { return m_colorTex; }
    unsigned int id()           const { return m_fbo; }
    int          width()        const { return m_width; }
    int          height()       const { return m_height; }

private:
    void create();
    void destroy();

    int          m_width{0};
    int          m_height{0};
    GLenum       m_format{GL_RGBA8};
    bool         m_hasDepth{false};
    unsigned int m_fbo{0};
    unsigned int m_colorTex{0};
    unsigned int m_depthRBO{0};
};

} // namespace eng::render
