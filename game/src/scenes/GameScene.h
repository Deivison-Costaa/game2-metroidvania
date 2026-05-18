#pragma once
#include "engine/animation/AnimationClip.h"
#include "engine/animation/AttackTable.h"
#include "engine/ecs/Registry.h"
#include "engine/map/TileMap.h"
#include "engine/physics/DebugDraw.h"
#include "engine/physics/PhysicsWorld.h"
#include "engine/render/Camera.h"
#include "engine/render/ParallaxRenderer.h"
#include "engine/render/ParticleSystem.h"
#include "engine/render/PostProcessStack.h"
#include "engine/render/SpriteBatch.h"
#include "engine/resources/ResourceManager.h"
#include "engine/render/Texture.h"
#include "engine/save/SaveSystem.h"
#include "systems/CameraSystem.h"
#include "systems/CombatSystem.h"
#include "data/BossAttackTable.h"
#include "ui/Hud.h"
#include <SDL2/SDL.h>
#include <memory>
#include <vector>

namespace eng::core { class App; }

class GameScene {
public:
    GameScene() = default;

    void init(eng::core::App& app);
    void reinit(eng::core::App& app); // full reset + re-init (used by Retry)
    void update(float dt, eng::core::App& app);
    void render();
    void handleEvent(const SDL_Event& ev);

    bool debugDrawOn() const noexcept { return m_debugDrawOn; }

    // Returns true after the player has been dead for 1.5s (triggers GameOver transition).
    bool playerDeadDelayElapsed() const noexcept { return m_playerDeadElapsed; }

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
    eng::animation::AttackTable m_attackTable;

    sys::CombatContactListener m_contactListener;
    sys::CameraState           m_camState;

    // Heap-allocated fixture user-data — owned here so they outlive the bodies
    std::vector<std::unique_ptr<sys::FixtureUserData>> m_fixtureData;

    eng::ecs::Entity m_player  {eng::ecs::kNullEntity};
    eng::ecs::Entity m_miniBoss{eng::ecs::kNullEntity};

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
    void spawnMiniBossFromObject(const eng::map::MapObject& obj);
    eng::ecs::Entity spawnProjectile(glm::vec2 from, glm::vec2 vel, float damage);

    // Save helpers
    void doSave();
    void doLoad();

    // M5 — Post-process stack + particles
    std::unique_ptr<eng::render::PostProcessStack> m_postFx;
    std::unique_ptr<eng::render::ParticleSystem>   m_particleSys;
    float m_dustTimer{0.f};  // accumulator for ambient dust emission

    // Sun world position for god rays (fixed artistic choice: upper-right background)
    static constexpr glm::vec3 kSunWorldPos{25.f, 10.f, -5.f};

    // M6 — HUD + Save + Boss AI table
    std::unique_ptr<Hud>          m_hud;
    data::BossAttackTable         m_bossTable;
    eng::save::SaveData           m_saveData;
    float                         m_playtime{0.f};

    float m_timeScale      {1.f};  // 0 during hit-stop, 1 normally
    float m_hitStopTimer   {0.f};
    float m_deathTimer     {0.f};  // accumulates after player dies
    bool  m_playerDeadElapsed{false}; // set true when m_deathTimer >= 1.5s
    bool  m_debugDrawOn    {false};
};
