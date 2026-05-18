#pragma once
#include "engine/render/Camera.h"
#include "engine/render/Shader.h"
#include "engine/render/Texture.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace eng::render {

enum class ParticleKind { DeathBurst, HitSpark, ProjectileTrail, Dust };

struct ParticleSpawnParams {
    glm::vec4 color{1.f};      // HDR color; components > 1 feed bloom
    float     sizeStart{0.2f};
    float     sizeEnd{0.0f};
    float     lifeMin{0.3f};
    float     lifeMax{0.8f};
    float     speedMin{1.0f};
    float     speedMax{3.0f};
    float     gravity{-2.0f};  // world-units / s²
    int       count{8};
};

class ParticleSystem {
public:
    static constexpr int kMaxParticles = 4096;

    ParticleSystem(const std::string& shaderDir, const std::string& texPath);
    ~ParticleSystem();
    ParticleSystem(const ParticleSystem&)            = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // High-level: spawn a preset burst at worldPos.
    // enemyKindHint only used for DeathBurst to choose color (0=Walker, 1=Flyer, 2=Ranged).
    void spawn(ParticleKind kind, const glm::vec3& worldPos, int enemyKindHint = 0);

    // Low-level: spawn with explicit params.
    void spawn(const glm::vec3& worldPos, const ParticleSpawnParams& p);

    void update(float dt);
    void render(const Camera& camera);

private:
    struct Particle {
        glm::vec3 pos{};
        glm::vec3 vel{};
        glm::vec4 colorStart{};
        float     life{0.f};
        float     lifeMax{1.f};
        float     sizeStart{0.2f};
        float     sizeEnd{0.0f};
        float     rot{0.f};
        float     rotVel{0.f};
        float     gravity{-2.f};
        bool      active{false};
    };

    struct InstanceData {
        glm::vec3 pos;
        float     _pad0{0.f};  // alignment
        glm::vec4 color;
        float     size;
        float     rot;
        float     _pad1[2]{};  // alignment
    };

    Particle* alloc();

    Shader       m_shader;
    Texture      m_tex;
    unsigned int m_vao{0};
    unsigned int m_cornerVBO{0};
    unsigned int m_instanceVBO{0};

    std::vector<Particle>     m_pool;
    std::vector<InstanceData> m_instanceData;
    int m_nextFree{0};
};

} // namespace eng::render
