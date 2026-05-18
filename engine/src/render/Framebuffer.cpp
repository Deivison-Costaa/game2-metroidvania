#include "engine/render/Framebuffer.h"
#include <stdexcept>

namespace eng::render {

Framebuffer::Framebuffer(int w, int h, GLenum colorFormat, bool depth)
    : m_width(w), m_height(h), m_format(colorFormat), m_hasDepth(depth)
{
    create();
}

Framebuffer::~Framebuffer() { destroy(); }

Framebuffer::Framebuffer(Framebuffer&& o) noexcept
    : m_width(o.m_width), m_height(o.m_height),
      m_format(o.m_format), m_hasDepth(o.m_hasDepth),
      m_fbo(o.m_fbo), m_colorTex(o.m_colorTex), m_depthRBO(o.m_depthRBO)
{
    o.m_fbo = o.m_colorTex = o.m_depthRBO = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& o) noexcept {
    if (this != &o) {
        destroy();
        m_width    = o.m_width;    m_height  = o.m_height;
        m_format   = o.m_format;   m_hasDepth = o.m_hasDepth;
        m_fbo      = o.m_fbo;      m_colorTex = o.m_colorTex;
        m_depthRBO = o.m_depthRBO;
        o.m_fbo    = o.m_colorTex = o.m_depthRBO = 0;
    }
    return *this;
}

void Framebuffer::bind()   const { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); }
void Framebuffer::unbind() const { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void Framebuffer::resize(int w, int h) {
    if (w == m_width && h == m_height) return;
    m_width = w;  m_height = h;
    destroy();
    create();
}

void Framebuffer::create() {
    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    glGenTextures(1, &m_colorTex);
    glBindTexture(GL_TEXTURE_2D, m_colorTex);
    // Use GL_RGBA as the base format for both RGBA8 and RGBA16F
    GLenum baseFormat = GL_RGBA;
    GLenum pixType    = (m_format == GL_RGBA16F) ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(m_format),
                 m_width, m_height, 0, baseFormat, pixType, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_colorTex, 0);

    if (m_hasDepth) {
        glGenRenderbuffers(1, &m_depthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER, m_depthRBO);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer: incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy() {
    if (m_colorTex) { glDeleteTextures(1, &m_colorTex);        m_colorTex = 0; }
    if (m_depthRBO) { glDeleteRenderbuffers(1, &m_depthRBO);   m_depthRBO = 0; }
    if (m_fbo)      { glDeleteFramebuffers(1, &m_fbo);         m_fbo      = 0; }
}

} // namespace eng::render
