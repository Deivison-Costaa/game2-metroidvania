#pragma once
#include "engine/render/Camera.h"
#include "engine/render/Framebuffer.h"
#include "engine/render/FullscreenQuad.h"
#include "engine/render/Shader.h"
#include <glm/glm.hpp>
#include <string>

namespace eng::render {

// HDR post-process pipeline: Bloom (dual-filter) + God Rays + ACES tonemap.
//
// Usage per frame:
//   stack.beginScene();          // bind HDR FBO — render your scene here
//   ... draw scene ...
//   stack.endSceneAndComposite(camera, sunWorldPos);  // bloom → godRays → composite → default FB
//
// After endSceneAndComposite(), default FB is bound. Render debug/UI on top.
class PostProcessStack {
public:
    PostProcessStack(int w, int h, const std::string& shaderDir);
    ~PostProcessStack() = default;
    PostProcessStack(const PostProcessStack&)            = delete;
    PostProcessStack& operator=(const PostProcessStack&) = delete;

    void beginScene();
    void endSceneAndComposite(const Camera& camera, const glm::vec3& sunWorldPos);
    void resize(int w, int h);

    // Tuning (safe defaults applied in ctor)
    void setBloomStrength(float v)          { m_bloomStrength  = v; }
    void setBloomThreshold(float v)         { m_bloomThreshold = v; }
    void setBlurIterations(int v)           { m_blurIter       = (v < 1 ? 1 : v > 5 ? 5 : v); }
    void setFogDensity(float v)             { m_fogDensity     = v; }
    void setFogColor(const glm::vec3& c)    { m_fogColor       = c; }
    void setExposure(float v)               { m_exposure       = v; }
    void setGodRayDensity(float v)          { m_rayDensity     = v; }
    void setGodRayStrength(float v)         { m_rayStrength    = v; }
    void setGodRayColor(const glm::vec3& c) { m_rayColor       = c; }

    float            fogDensity()  const { return m_fogDensity; }
    const glm::vec3& fogColor()    const { return m_fogColor; }
    float            bloomStrength() const { return m_bloomStrength; }
    float            exposure()    const { return m_exposure; }
    int              blurIterations() const { return m_blurIter; }

private:
    void doBrightPass();
    void doGodRays(const glm::vec2& sunUV);
    void doBlur();
    void doComposite();

    int m_width, m_height;

    // FBOs: hdr=full-res, ping/pong=half-res, rays=quarter-res
    Framebuffer m_hdrFBO;
    Framebuffer m_pingFBO;
    Framebuffer m_pongFBO;
    Framebuffer m_raysFBO;

    FullscreenQuad m_quad;

    Shader m_brightPassSh;
    Shader m_blurSh;
    Shader m_godRaysSh;
    Shader m_compositeSh;

    // Parameters with conservative visual defaults
    float     m_bloomStrength{0.04f};
    float     m_bloomThreshold{1.0f};
    int       m_blurIter{3};
    float     m_fogDensity{0.3f};
    glm::vec3 m_fogColor{0.05f, 0.04f, 0.08f};
    float     m_exposure{1.0f};
    float     m_rayDensity{0.96f};
    float     m_rayStrength{0.25f};
    glm::vec3 m_rayColor{1.0f, 0.85f, 0.5f};
};

} // namespace eng::render
