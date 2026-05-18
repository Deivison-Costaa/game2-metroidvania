#pragma once
#include "engine/animation/AnimationClip.h"
#include "engine/ecs/Registry.h"
#include "engine/map/TileMap.h"
#include "engine/physics/DebugDraw.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/render/Camera.h"
#include "engine/render/ParallaxRenderer.h"
#include "engine/render/SpriteBatch.h"
#include "engine/resources/ResourceManager.h"
#include "engine/render/Texture.h"
#include "systems/CameraSystem.h"
#include "systems/CombatSystem.h"
#include <SDL2/SDL.h>
#include <memory>
#include <vector>

namespace eng::core { class App; }

class GameScene {
public:
    GameScene() = default;

    void init(eng::core::App& app);
    void update(float dt, eng::core::App& app);
    void render();
    void handleEvent(const SDL_Event& ev);

    bool debugDrawOn() const noexcept { return m_debugDrawOn; }

    void requestHitStop(float duration) {
        if (duration > m_hitStopTimer) m_hitStopTimer = duration;
    }

private:
    void buildLevel();

    eng::ecs::Registry   m_reg;
    eng::physics::PhysicsWorld m_physics;
    eng::render::Camera  m_camera{
        eng::render::Camera::perspective(glm::radians(30.f), 16.f / 9.f, 0.1f, 200.f)};

    eng::render::SpriteBatch*  m_batch   {nullptr};
    eng::physics::DebugDraw*   m_dbgDraw {nullptr};

    std::unique_ptr<eng::render::SpriteBatch> m_batchOwner;
    std::unique_ptr<eng::physics::DebugDraw>  m_dbgDrawOwner;

    eng::resources::ResourceManager<eng::render::Texture>         m_textures;
    eng::resources::ResourceManager<eng::animation::AnimationClip> m_clips;

    sys::CombatContactListener m_contactListener;
    sys::CameraState           m_camState;

    // Heap-allocated fixture user-data — owned here so they outlive the bodies
    std::vector<std::unique_ptr<sys::FixtureUserData>> m_fixtureData;

    eng::ecs::Entity m_player{eng::ecs::kNullEntity};

    // M4 — TileMap + Parallax + Enemies
    std::unique_ptr<eng::map::TileMap>      m_tileMap;
    std::shared_ptr<eng::render::Texture>   m_tilesetTex;
    std::vector<eng::render::ParallaxLayer> m_parallax;
    std::vector<eng::ecs::Entity>           m_enemies;

    // Offset that maps the TMX bottom-left to world (0,0)
    // Ground-top surface sits at world y=0 with this value
    static constexpr glm::vec2 kMapOrigin{-20.f, -2.f};

    // Spawn helpers (called from buildLevel after TMX is loaded)
    void spawnEnemyFromObject(const eng::map::MapObject& obj);
    eng::ecs::Entity spawnProjectile(glm::vec2 from, glm::vec2 vel, float damage);

    float m_timeScale   {1.f};  // 0 during hit-stop, 1 normally
    float m_hitStopTimer{0.f};
    bool  m_debugDrawOn {false};
};
