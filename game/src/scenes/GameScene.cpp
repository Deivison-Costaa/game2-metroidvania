#include "scenes/GameScene.h"
#include "components/PlayerControl.h"
#include "components/RigidBody.h"
#include "components/SpriteRenderer.h"
#include "components/Transform.h"
#include "systems/PhysicsSyncSystem.h"
#include "systems/PlayerControllerSystem.h"
#include "systems/SpriteRenderSystem.h"
#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Action.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

// ── Scene layout (in meters) ──────────────────────────────────────────────────
//  Ground: center (0, -0.5),  half (8, 0.5)  — top at y=0
//  Left wall:  (-8, 4), half (0.12, 5)
//  Right wall: ( 8, 4), half (0.12, 5)
//  Player: start (0, 2), half (0.25, 0.5)    — 0.5m wide, 1m tall

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

    auto tex = m_textures.load("player",
        ASSET_ROOT "/assets/sprites/test.png",
        eng::render::TextureFilter::Nearest);

    m_physics.world().SetDebugDraw(m_dbgDraw);

    buildLevel();

    // Set player sprite texture
    auto& sr = m_reg.get<SpriteRenderer>(m_player);
    sr.tex  = tex;
    sr.size = {0.5f, 0.5f};  // 1m x 1m displayed quad

    LOG_INFO("M2 ready — A/D move | Space jump | F1 debug | ESC quit");
}

void GameScene::buildLevel() {
    // Static world bodies
    m_physics.createStaticBox({0.f, -0.5f},  8.f,  0.5f);  // ground
    m_physics.createStaticBox({-8.f, 4.f},  0.12f, 5.f);   // left wall
    m_physics.createStaticBox({ 8.f, 4.f},  0.12f, 5.f);   // right wall

    // Player entity
    m_player = m_reg.create();
    b2Body* body = m_physics.createDynamicBox({0.f, 2.f}, 0.25f, 0.5f, 1.f, 0.1f);
    body->SetLinearDamping(0.f);

    m_reg.emplace<Transform>(m_player);
    m_reg.emplace<RigidBody>(m_player, RigidBody{body, 0.25f, 0.5f});
    m_reg.emplace<SpriteRenderer>(m_player);
    m_reg.emplace<PlayerControl>(m_player);

    m_camX = 0.f;
}

void GameScene::update(float dt, eng::core::App& app) {
    auto& input = app.input();

    // Toggle debug draw
    if (input.pressed(eng::input::Action::ToggleDebug))
        m_debugDrawOn = !m_debugDrawOn;

    // Systems
    sys::playerControllerUpdate(m_reg, input, m_physics);
    m_physics.step(dt);
    sys::physicsSyncUpdate(m_reg);

    // Camera follow player on X axis
    if (m_reg.valid(m_player)) {
        const auto& t = m_reg.get<Transform>(m_player);
        m_camX += (t.position.x - m_camX) * 5.f * dt;
    }

    // Camera: FoV 30° looking along -Z; positioned at (camX, 4, 16)
    const float camY = 3.5f;
    const float camZ = 16.f;
    m_camera.setPosition({m_camX, camY, camZ});
    m_camera.setTarget({m_camX, camY, 0.f});
    m_camera.setUp({0.f, 1.f, 0.f});
}

void GameScene::render() {
    glClearColor(0.05f, 0.04f, 0.08f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Sprite pass
    m_batch->begin(m_camera);
    sys::spriteRenderUpdate(m_reg, *m_batch);
    m_batch->end();

    // Debug draw (toggle with F1)
    if (m_debugDrawOn) {
        m_physics.world().DebugDraw();
        m_dbgDraw->render(m_camera);
    }
}

void GameScene::handleEvent(const SDL_Event& ev) {
    // Window resize → update camera aspect
    if (ev.type == SDL_WINDOWEVENT &&
        ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        const float w = static_cast<float>(ev.window.data1);
        const float h = static_cast<float>(ev.window.data2);
        m_camera.setAspect(w / h);
    }
}
