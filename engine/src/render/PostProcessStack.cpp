#include "engine/render/PostProcessStack.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

namespace eng::render {

PostProcessStack::PostProcessStack(int w, int h, const std::string& sd)
    : m_width(w), m_height(h),
      m_hdrFBO (w,     h,     GL_RGBA16F, true),
      m_pingFBO(w / 2, h / 2, GL_RGBA16F, false),
      m_pongFBO(w / 2, h / 2, GL_RGBA16F, false),
      m_raysFBO(w / 4, h / 4, GL_RGBA16F, false),
      m_brightPassSh(Shader::fromFiles(sd + "/post_fullscreen.vert",
                                       sd + "/bright_pass.frag")),
      m_blurSh      (Shader::fromFiles(sd + "/post_fullscreen.vert",
                                       sd + "/blur_dual_filter.frag")),
      m_godRaysSh   (Shader::fromFiles(sd + "/post_fullscreen.vert",
                                       sd + "/god_rays.frag")),
      m_compositeSh (Shader::fromFiles(sd + "/post_fullscreen.vert",
                                       sd + "/composite.frag"))
{
    LOG_INFO("PostProcessStack: HDR {}x{} RGBA16F | bloom {}x{} | god rays {}x{}",
             w, h, w/2, h/2, w/4, h/4);
}

void PostProcessStack::resize(int w, int h) {
    if (w == m_width && h == m_height) return;
    m_width  = w;
    m_height = h;
    m_hdrFBO .resize(w,     h    );
    m_pingFBO.resize(w / 2, h / 2);
    m_pongFBO.resize(w / 2, h / 2);
    m_raysFBO.resize(w / 4, h / 4);
}

void PostProcessStack::beginScene() {
    m_hdrFBO.bind();
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.05f, 0.04f, 0.08f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcessStack::endSceneAndComposite(const Camera& camera,
                                            const glm::vec3& sunWorldPos)
{
    // Project sun to screen UV [0,1]
    const glm::vec4 sunClip = camera.viewProjection() * glm::vec4(sunWorldPos, 1.f);
    glm::vec2 sunUV{0.5f};
    if (sunClip.w > 0.f) {
        const glm::vec3 ndc = glm::vec3(sunClip) / sunClip.w;
        sunUV = glm::vec2(ndc.x, ndc.y) * 0.5f + 0.5f;
    }

    // Disable depth test for fullscreen passes
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    doBrightPass();
    doGodRays(sunUV);
    doBlur();
    doComposite();

    // Restore scene GL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
}

// ── Passes ────────────────────────────────────────────────────────────────────

void PostProcessStack::doBrightPass() {
    // Render to half-res pingFBO; downsample + extract bright pixels
    m_pingFBO.bind();
    glViewport(0, 0, m_width / 2, m_height / 2);
    glClear(GL_COLOR_BUFFER_BIT);

    m_brightPassSh.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrFBO.colorTexture());
    m_brightPassSh.set("uHDR",       0);
    m_brightPassSh.set("uThreshold", m_bloomThreshold);
    m_quad.draw();
}

void PostProcessStack::doGodRays(const glm::vec2& sunUV) {
    // Radial blur of bright-pass into quarter-res raysFBO
    m_raysFBO.bind();
    glViewport(0, 0, m_width / 4, m_height / 4);
    glClear(GL_COLOR_BUFFER_BIT);

    m_godRaysSh.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_pingFBO.colorTexture()); // bright-pass
    m_godRaysSh.set("uBrightPass", 0);
    m_godRaysSh.set("uSunUV",      sunUV);
    m_godRaysSh.set("uDensity",    m_rayDensity);
    m_godRaysSh.set("uWeight",     0.1f);
    m_godRaysSh.set("uDecay",      0.95f);
    m_godRaysSh.set("uExposure",   m_rayStrength);
    m_godRaysSh.set("uColor",      m_rayColor);
    m_quad.draw();
}

void PostProcessStack::doBlur() {
    // Dual-filter blur ping-pong at half-res — m_blurIter pairs (down+up each)
    // After each pair the result is back in pingFBO
    const glm::vec2 halfTexel{1.f / static_cast<float>(m_width  / 2),
                               1.f / static_cast<float>(m_height / 2)};

    m_blurSh.bind();
    m_blurSh.set("uTexelSize", halfTexel);

    for (int i = 0; i < m_blurIter; ++i) {
        // Downsample: pingFBO → pongFBO
        m_pongFBO.bind();
        glViewport(0, 0, m_width / 2, m_height / 2);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_pingFBO.colorTexture());
        m_blurSh.set("uTex",  0);
        m_blurSh.set("uMode", 0);
        m_quad.draw();

        // Upsample: pongFBO → pingFBO
        m_pingFBO.bind();
        glViewport(0, 0, m_width / 2, m_height / 2);
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_pongFBO.colorTexture());
        m_blurSh.set("uTex",  0);
        m_blurSh.set("uMode", 1);
        m_quad.draw();
    }
    // Final bloom result is in pingFBO
}

void PostProcessStack::doComposite() {
    // Composite HDR + bloom + god rays → default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);

    m_compositeSh.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrFBO.colorTexture());
    m_compositeSh.set("uHDR", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_pingFBO.colorTexture());
    m_compositeSh.set("uBloom", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_raysFBO.colorTexture());
    m_compositeSh.set("uGodRays", 2);

    m_compositeSh.set("uBloomStrength", m_bloomStrength);
    m_compositeSh.set("uExposure",      m_exposure);
    m_compositeSh.set("uTexelSize",
        glm::vec2{1.f / static_cast<float>(m_width),
                  1.f / static_cast<float>(m_height)});

    m_quad.draw();
    glActiveTexture(GL_TEXTURE0);
}

} // namespace eng::render
