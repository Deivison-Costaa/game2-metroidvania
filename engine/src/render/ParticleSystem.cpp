#include "engine/render/ParticleSystem.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <cmath>
#include <cstdlib>

namespace eng::render {

// ── helpers ───────────────────────────────────────────────────────────────────

static float randf(float lo, float hi) {
    return lo + static_cast<float>(rand()) / RAND_MAX * (hi - lo);
}

static glm::vec3 randDir2D() {
    const float a = randf(0.f, 6.2832f);
    return {std::cos(a), std::sin(a), 0.f};
}

// ── ctor / dtor ───────────────────────────────────────────────────────────────

ParticleSystem::ParticleSystem(const std::string& shaderDir, const std::string& texPath)
    : m_shader(Shader::fromFiles(shaderDir + "/particle.vert",
                                 shaderDir + "/particle.frag")),
      m_tex(Texture::fromFile(texPath, TextureFilter::Linear))
{
    m_pool.resize(kMaxParticles);
    m_instanceData.reserve(kMaxParticles);

    // Quad corners for triangle-strip: TL, BL, TR, BR  (pos.xy, uv.xy)
    const float corners[] = {
        -0.5f,  0.5f,  0.f, 1.f,
        -0.5f, -0.5f,  0.f, 0.f,
         0.5f,  0.5f,  1.f, 1.f,
         0.5f, -0.5f,  1.f, 0.f,
    };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_cornerVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cornerVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);

    // loc 0: corner pos (vec2), loc 1: corner UV (vec2)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));

    glGenBuffers(1, &m_instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, kMaxParticles * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

    const int stride = sizeof(InstanceData);
    // loc 2: pos (vec3), loc 3: color (vec4), loc 4: size (float), loc 5: rot (float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(InstanceData, pos)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(InstanceData, color)));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(InstanceData, size)));
    glVertexAttribDivisor(4, 1);

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(InstanceData, rot)));
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);

    LOG_INFO("ParticleSystem: pool={} | spark texture loaded", kMaxParticles);
}

ParticleSystem::~ParticleSystem() {
    if (m_cornerVBO)   glDeleteBuffers(1, &m_cornerVBO);
    if (m_instanceVBO) glDeleteBuffers(1, &m_instanceVBO);
    if (m_vao)         glDeleteVertexArrays(1, &m_vao);
}

// ── allocation ────────────────────────────────────────────────────────────────

ParticleSystem::Particle* ParticleSystem::alloc() {
    // Linear scan from nextFree; wraps if pool exhausted (LRU overwrite)
    for (int i = 0; i < kMaxParticles; ++i) {
        int idx = (m_nextFree + i) % kMaxParticles;
        if (!m_pool[idx].active) {
            m_nextFree = (idx + 1) % kMaxParticles;
            return &m_pool[idx];
        }
    }
    // Pool full — overwrite oldest (nextFree)
    Particle* p = &m_pool[m_nextFree];
    m_nextFree  = (m_nextFree + 1) % kMaxParticles;
    return p;
}

// ── spawn ─────────────────────────────────────────────────────────────────────

void ParticleSystem::spawn(const glm::vec3& worldPos, const ParticleSpawnParams& p) {
    for (int i = 0; i < p.count; ++i) {
        Particle* pt = alloc();
        pt->pos       = worldPos;
        pt->vel       = randDir2D() * randf(p.speedMin, p.speedMax);
        pt->colorStart = p.color;
        pt->lifeMax   = randf(p.lifeMin, p.lifeMax);
        pt->life      = pt->lifeMax;
        pt->sizeStart = p.sizeStart;
        pt->sizeEnd   = p.sizeEnd;
        pt->rot       = randf(0.f, 6.2832f);
        pt->rotVel    = randf(-3.f, 3.f);
        pt->gravity   = p.gravity;
        pt->active    = true;
    }
}

void ParticleSystem::spawn(ParticleKind kind, const glm::vec3& worldPos, int enemyKindHint) {
    switch (kind) {
    case ParticleKind::DeathBurst: {
        ParticleSpawnParams p;
        p.count      = 10;
        p.speedMin   = 2.f;  p.speedMax  = 5.f;
        p.lifeMin    = 0.4f; p.lifeMax   = 0.9f;
        p.sizeStart  = 0.25f; p.sizeEnd  = 0.0f;
        p.gravity    = -1.5f;
        // Color by enemy type (HDR > 1 feeds bloom)
        if      (enemyKindHint == 0) p.color = {2.0f, 0.3f, 0.3f, 1.f}; // Walker: red
        else if (enemyKindHint == 1) p.color = {0.3f, 2.0f, 1.8f, 1.f}; // Flyer: cyan
        else                         p.color = {1.5f, 0.3f, 2.0f, 1.f}; // Ranged: purple
        spawn(worldPos, p);
        break;
    }
    case ParticleKind::HitSpark: {
        ParticleSpawnParams p;
        p.count      = 6;
        p.speedMin   = 3.f;  p.speedMax  = 6.f;
        p.lifeMin    = 0.1f; p.lifeMax   = 0.25f;
        p.sizeStart  = 0.15f; p.sizeEnd  = 0.0f;
        p.gravity    = 0.f;
        p.color      = {2.5f, 2.0f, 1.2f, 1.f}; // bright white-yellow HDR
        spawn(worldPos, p);
        break;
    }
    case ParticleKind::ProjectileTrail: {
        ParticleSpawnParams p;
        p.count      = 1;
        p.speedMin   = 0.1f; p.speedMax  = 0.3f;
        p.lifeMin    = 0.08f; p.lifeMax  = 0.15f;
        p.sizeStart  = 0.12f; p.sizeEnd  = 0.0f;
        p.gravity    = 0.f;
        p.color      = {2.0f, 0.2f, 1.8f, 1.f}; // magenta HDR
        spawn(worldPos, p);
        break;
    }
    case ParticleKind::Dust: {
        ParticleSpawnParams p;
        p.count      = 1;
        p.speedMin   = 0.05f; p.speedMax = 0.2f;
        p.lifeMin    = 2.0f;  p.lifeMax  = 4.0f;
        p.sizeStart  = 0.12f; p.sizeEnd  = 0.0f;
        p.gravity    = 0.05f; // gentle float upward (positive = up)
        p.color      = {0.5f, 0.45f, 0.6f, 0.4f}; // dim grey-blue
        spawn(worldPos, p);
        break;
    }
    }
}

// ── update ────────────────────────────────────────────────────────────────────

void ParticleSystem::update(float dt) {
    for (Particle& p : m_pool) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.f) { p.active = false; continue; }
        p.vel.y += p.gravity * dt;
        p.pos   += p.vel * dt;
        p.rot   += p.rotVel * dt;
    }
}

// ── render ────────────────────────────────────────────────────────────────────

void ParticleSystem::render(const Camera& camera) {
    m_instanceData.clear();
    for (const Particle& p : m_pool) {
        if (!p.active) continue;
        const float t    = 1.f - p.life / p.lifeMax; // 0=born, 1=dead
        const float sz   = p.sizeStart + (p.sizeEnd - p.sizeStart) * t;
        const float alpha = (1.f - t) * p.colorStart.a;
        m_instanceData.push_back({
            p.pos, 0.f,
            {p.colorStart.r, p.colorStart.g, p.colorStart.b, alpha},
            sz, p.rot
        });
    }

    if (m_instanceData.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(m_instanceData.size() * sizeof(InstanceData)),
                    m_instanceData.data());

    m_shader.bind();
    m_shader.set("uViewProj", camera.viewProjection());
    m_tex.bind(0);
    m_shader.set("uTex", 0);

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4,
                          static_cast<GLsizei>(m_instanceData.size()));
    glBindVertexArray(0);
}

} // namespace eng::render
