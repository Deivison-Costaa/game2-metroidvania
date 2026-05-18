#pragma once
#include "engine/ecs/Registry.h"
#include "engine/physics/DebugDraw.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/render/Camera.h"
#include "engine/render/SpriteBatch.h"
#include "engine/resources/ResourceManager.h"
#include "engine/render/Texture.h"
#include <SDL2/SDL.h>

namespace eng::core { class App; }

class GameScene {
public:
    GameScene() = default;

    void init(eng::core::App& app);
    void update(float dt, eng::core::App& app);
    void render();
    void handleEvent(const SDL_Event& ev);

    bool debugDrawOn() const noexcept { return m_debugDrawOn; }

private:
    void buildLevel();

    eng::ecs::Registry                              m_reg;
    eng::physics::PhysicsWorld                      m_physics;
    eng::render::Camera                             m_camera{
        eng::render::Camera::perspective(glm::radians(30.f), 16.f / 9.f, 0.1f, 200.f)};
    eng::render::SpriteBatch*                       m_batch{nullptr};
    eng::physics::DebugDraw*                        m_dbgDraw{nullptr};
    eng::resources::ResourceManager<eng::render::Texture> m_textures;

    // Owned heap objects (stored separately so Camera stays copyable)
    std::unique_ptr<eng::render::SpriteBatch>  m_batchOwner;
    std::unique_ptr<eng::physics::DebugDraw>   m_dbgDrawOwner;

    eng::ecs::Entity m_player{eng::ecs::kNullEntity};

    float m_camX{0.f};    // camera follow target X
    bool  m_debugDrawOn{false};
};
