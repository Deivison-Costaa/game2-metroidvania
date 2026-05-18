#include "scenes/GameScene.h"
#include "components/Animator.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Hurtbox.h"
#include "components/PlayerControl.h"
#include "components/RigidBody.h"
#include "components/SpriteRenderer.h"
#include "components/Transform.h"
#include "systems/AnimationSystem.h"
#include "systems/CameraSystem.h"
#include "systems/CombatSystem.h"
#include "systems/PhysicsSyncSystem.h"
#include "systems/PlayerControllerSystem.h"
#include "systems/SpriteRenderSystem.h"
#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Action.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// Fixture user-data helper — allocate once, keep alive in m_fixtureData
static sys::FixtureUserData* makeFixtureUD(
    std::vector<std::unique_ptr<sys::FixtureUserData>>& vec,
    sys::FixtureTag tag,
    eng::ecs::Entity entity)
{
    auto& ptr = vec.emplace_back(std::make_unique<sys::FixtureUserData>());
    ptr->tag    = tag;
    ptr->entity = entity;
    return ptr.get();
}

// ── Scene layout (in meters) ──────────────────────────────────────────────────
//  Ground:     (0,  -0.5),  half (8.0, 0.5)    — top surface at y=0
//  Left wall:  (-8,  4.0),  half (0.12, 5.0)
//  Right wall: ( 8,  4.0),  half (0.12, 5.0)
//  Player:     start (0,  2.0), half (0.25, 0.5) — 0.5m wide, 1m tall
//  Dummy:      start (3,  0.5), half (0.30, 0.5) — 0.6m wide, 1m tall

void GameScene::init(eng::core::App& app) {
    const float w = static_cast<float>(app.window().width());
    const float h = static_cast<float>(app.window().height());
    m_camera = eng::render::Camera::perspective(glm::radians(30.f), w / h, 0.1f, 200.f);

    m_batchOwner = std::make_unique<eng::render::SpriteBatch>(
        ASSET_ROOT "/shaders/sprite.vert", ASSET_ROOT "/shaders/sprite.frag");
    m_batch = m_batchOwner.get();

    m_dbgDrawOwner = std::make_unique<eng::physics::DebugDraw>(
        ASSET_ROOT "/shaders/debug_line.vert", ASSET_ROOT "/shaders/debug_line.frag");
    m_dbgDraw = m_dbgDrawOwner.get();

    // Textures
    m_textures.load("player_sheet",
        ASSET_ROOT "/assets/sprites/player_sheet.png",
        eng::render::TextureFilter::Nearest);
    m_textures.load("dummy",
        ASSET_ROOT "/assets/sprites/dummy.png",
        eng::render::TextureFilter::Nearest);

    // Animation clips
    m_clips.load("idle",   ASSET_ROOT "/assets/data/player_idle.json");
    m_clips.load("run",    ASSET_ROOT "/assets/data/player_run.json");
    m_clips.load("jump",   ASSET_ROOT "/assets/data/player_jump.json");
    m_clips.load("fall",   ASSET_ROOT "/assets/data/player_fall.json");
    m_clips.load("attack", ASSET_ROOT "/assets/data/player_attack.json");

    // Physics debug draw
    m_physics.world().SetDebugDraw(m_dbgDraw);

    // Combat contact listener
    m_physics.setContactListener(&m_contactListener);

    // Camera initial state
    m_camState = sys::CameraState{};

    buildLevel();

    LOG_INFO("M3 ready — A/D move | Space jump | J attack | F1 debug | ESC quit");
}

void GameScene::buildLevel() {
    // --- Static world geometry ---
    m_physics.createStaticBox({0.f, -0.5f},  8.f,  0.5f);   // ground
    m_physics.createStaticBox({-8.f, 4.f},  0.12f, 5.f);    // left wall
    m_physics.createStaticBox({ 8.f, 4.f},  0.12f, 5.f);    // right wall

    // --- Player ---
    m_player = m_reg.create();
    b2Body* playerBody = m_physics.createDynamicBox({0.f, 2.f}, 0.25f, 0.5f, 1.f, 0.1f);
    playerBody->SetLinearDamping(0.f);

    m_reg.emplace<Transform>(m_player);
    m_reg.emplace<RigidBody>(m_player, RigidBody{playerBody, 0.25f, 0.5f});

    // Sprite — full 1m x 1m quad showing the sprite sheet
    auto& psr  = m_reg.emplace<SpriteRenderer>(m_player);
    psr.tex    = m_textures.get("player_sheet");
    psr.size   = {0.9f, 0.9f};

    m_reg.emplace<PlayerControl>(m_player);

    // Animator — start in idle
    auto& anim = m_reg.emplace<Animator>(m_player);
    anim.clip  = m_clips.get("idle");

    // Hitbox sensor — always active in Box2D, gated by Hitbox.active in CombatSystem
    b2Fixture* hitFix = m_physics.addSensorBox(
        playerBody, {0.6f, 0.f}, 0.4f, 0.3f);
    hitFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hitbox, m_player));

    auto& hitbox      = m_reg.emplace<Hitbox>(m_player);
    hitbox.fixture    = hitFix;
    hitbox.damage     = 1.f;
    hitbox.knockback  = {4.f, 1.5f};

    // --- Training dummy ---
    m_dummy = m_reg.create();
    // Dynamic with high damping so it absorbs knockback visually then settles
    b2Body* dummyBody = m_physics.createDynamicBox({3.f, 0.6f}, 0.3f, 0.5f, 5.f, 0.8f);
    dummyBody->SetLinearDamping(4.f);
    dummyBody->SetFixedRotation(true);

    m_reg.emplace<Transform>(m_dummy);
    m_reg.emplace<RigidBody>(m_dummy, RigidBody{dummyBody, 0.3f, 0.5f});

    auto& dsr = m_reg.emplace<SpriteRenderer>(m_dummy);
    dsr.tex   = m_textures.get("dummy");
    dsr.size  = {0.6f, 1.0f};

    m_reg.emplace<Health>(m_dummy, Health{3.f, 3.f});

    // Hurtbox sensor on dummy body
    b2Fixture* hurtFix = m_physics.addSensorBox(
        dummyBody, {0.f, 0.f}, 0.3f, 0.5f);
    hurtFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hurtbox, m_dummy));

    auto& hurtbox   = m_reg.emplace<Hurtbox>(m_dummy);
    hurtbox.fixture = hurtFix;
    hurtbox.owner   = m_dummy;

    // Player start camera state
    m_camState.pos      = {0.f, 3.5f};
    m_camState.camTarget = {0.f, 3.5f};
}

void GameScene::update(float dt, eng::core::App& app) {
    auto& input = app.input();

    // --- Hit-stop time scaling ---
    m_hitStopTimer = std::max(0.f, m_hitStopTimer - dt);
    m_timeScale    = (m_hitStopTimer > 0.f) ? 0.f : 1.f;
    const float gdt = dt * m_timeScale; // scaled gameplay dt

    // Toggle debug draw
    if (input.pressed(eng::input::Action::ToggleDebug))
        m_debugDrawOn = !m_debugDrawOn;

    // --- Gameplay systems (use scaled dt) ---
    sys::playerControllerUpdate(m_reg, input, m_physics, gdt, m_clips);
    sys::animationUpdate(m_reg, gdt);
    sys::combatPreUpdate(m_reg, gdt);
    m_physics.step(gdt);
    sys::physicsSyncUpdate(m_reg);

    // --- Combat post (resolve hits, apply callbacks) ---
    auto onHitStop = [this](float dur) { requestHitStop(dur); };
    auto onTrauma  = [this](float amt) { sys::cameraAddTrauma(m_camState, amt); };
    sys::combatPostUpdate(m_reg, m_contactListener, onHitStop, onTrauma);

    // --- Camera (uses real dt — continues animating during hit-stop) ---
    if (m_reg.valid(m_player)) {
        const auto& t    = m_reg.get<Transform>(m_player);
        const auto& ctrl = m_reg.get<PlayerControl>(m_player);
        const bool  atk  = (ctrl.state == PlayerState::Attack);
        sys::cameraUpdate(m_camState, t.position, ctrl.facing, atk, dt, m_camera);
    }
}

void GameScene::render() {
    glClearColor(0.05f, 0.04f, 0.08f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sprite pass
    m_batch->begin(m_camera);
    sys::spriteRenderUpdate(m_reg, *m_batch);
    m_batch->end();

    // Box2D debug draw (F1 toggle — shows hitbox/hurtbox sensors too)
    if (m_debugDrawOn) {
        m_physics.world().DebugDraw();
        m_dbgDraw->render(m_camera);
    }
}

void GameScene::handleEvent(const SDL_Event& ev) {
    if (ev.type == SDL_WINDOWEVENT &&
        ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        const float w = static_cast<float>(ev.window.data1);
        const float h = static_cast<float>(ev.window.data2);
        m_camera.setAspect(w / h);
    }
}
