#include "scenes/GameScene.h"
#include "components/Animator.h"
#include "components/EnemyAI.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Hurtbox.h"
#include "components/PlayerControl.h"
#include "components/Projectile.h"
#include "components/RigidBody.h"
#include "components/SpriteRenderer.h"
#include "components/Transform.h"
#include "systems/AnimationSystem.h"
#include "systems/CameraSystem.h"
#include "systems/CombatSystem.h"
#include "systems/EnemyAISystem.h"
#include "systems/EnemyCombatSystem.h"
#include "systems/PhysicsSyncSystem.h"
#include "systems/PlayerControllerSystem.h"
#include "systems/ProjectileSystem.h"
#include "systems/SpriteRenderSystem.h"
#include "engine/audio/AudioEngine.h"
#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Action.h"
#include "engine/map/TileMapRenderer.h"
#include "engine/save/SaveSystem.h"
#include "components/BossPhaseData.h"
#include "systems/BossLogic.h"
#include "data/BossAttackTable.h"
#include "engine/physics/CollisionCategories.h"
#include "engine/physics/PhysicsConstants.h"
#include "engine/render/ParallaxRenderer.h"
#include "engine/render/Texture3D.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cstdlib>

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
    const int   wi = app.window().width();
    const int   hi = app.window().height();
    const float w  = static_cast<float>(wi);
    const float h  = static_cast<float>(hi);
    m_camera = eng::render::Camera::perspective(glm::radians(30.f), w / h, 0.1f, 200.f);

    m_batchOwner = std::make_unique<eng::render::SpriteBatch>(
        ASSET_ROOT "/shaders/sprite.vert", ASSET_ROOT "/shaders/sprite.frag");
    m_batch = m_batchOwner.get();

    // M5 — Post-process stack (HDR FBO + bloom + god rays + ACES) + particles
    m_postFx = std::make_unique<eng::render::PostProcessStack>(
        wi, hi, std::string(ASSET_ROOT) + "/shaders");
    m_particleSys = std::make_unique<eng::render::ParticleSystem>(
        std::string(ASSET_ROOT) + "/shaders",
        std::string(ASSET_ROOT) + "/assets/sprites/particle_spark.png");

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
    m_textures.load("enemy_walker",
        ASSET_ROOT "/assets/sprites/enemy_walker.png",
        eng::render::TextureFilter::Nearest);
    m_textures.load("enemy_flyer",
        ASSET_ROOT "/assets/sprites/enemy_flyer.png",
        eng::render::TextureFilter::Nearest);
    m_textures.load("enemy_ranged",
        ASSET_ROOT "/assets/sprites/enemy_ranged.png",
        eng::render::TextureFilter::Nearest);

    // Animation clips
    m_clips.load("idle",   ASSET_ROOT "/assets/data/player_idle.json");
    m_clips.load("run",    ASSET_ROOT "/assets/data/player_run.json");
    m_clips.load("jump",   ASSET_ROOT "/assets/data/player_jump.json");
    m_clips.load("fall",   ASSET_ROOT "/assets/data/player_fall.json");
    m_clips.load("attack", ASSET_ROOT "/assets/data/player_attack.json");

    // Enemy Walker clips
    m_clips.load("walker_patrol", ASSET_ROOT "/assets/data/enemy_walker_patrol.json");
    m_clips.load("walker_chase",  ASSET_ROOT "/assets/data/enemy_walker_chase.json");
    m_clips.load("walker_attack", ASSET_ROOT "/assets/data/enemy_walker_attack.json");
    m_clips.load("walker_hurt",   ASSET_ROOT "/assets/data/enemy_walker_hurt.json");
    m_clips.load("walker_dead",   ASSET_ROOT "/assets/data/enemy_walker_dead.json");
    // Enemy Flyer clips
    m_clips.load("flyer_patrol",  ASSET_ROOT "/assets/data/enemy_flyer_patrol.json");
    m_clips.load("flyer_chase",   ASSET_ROOT "/assets/data/enemy_flyer_chase.json");
    m_clips.load("flyer_attack",  ASSET_ROOT "/assets/data/enemy_flyer_attack.json");
    m_clips.load("flyer_hurt",    ASSET_ROOT "/assets/data/enemy_flyer_hurt.json");
    m_clips.load("flyer_dead",    ASSET_ROOT "/assets/data/enemy_flyer_dead.json");
    // Enemy Ranged clips
    m_clips.load("ranged_idle",   ASSET_ROOT "/assets/data/enemy_ranged_patrol.json");
    m_clips.load("ranged_attack", ASSET_ROOT "/assets/data/enemy_ranged_attack.json");
    m_clips.load("ranged_hurt",   ASSET_ROOT "/assets/data/enemy_ranged_hurt.json");
    m_clips.load("ranged_dead",   ASSET_ROOT "/assets/data/enemy_ranged_dead.json");

    // Boss texture + clips
    m_textures.load("boss",
        ASSET_ROOT "/assets/sprites/boss.png",
        eng::render::TextureFilter::Nearest);
    m_clips.load("boss_stance_idle",       ASSET_ROOT "/assets/data/boss_stance_idle.json");
    m_clips.load("boss_stance_slash",      ASSET_ROOT "/assets/data/boss_stance_slash.json");
    m_clips.load("boss_enrage_slash",      ASSET_ROOT "/assets/data/boss_enrage_slash.json");
    m_clips.load("boss_desperate_radial",  ASSET_ROOT "/assets/data/boss_desperate_radial.json");
    m_clips.load("boss_hurt",              ASSET_ROOT "/assets/data/boss_hurt.json");
    m_clips.load("boss_dead",              ASSET_ROOT "/assets/data/boss_dead.json");
    m_bossTable   = data::BossAttackTable::fromFile(
        ASSET_ROOT "/assets/data/boss_attacks.json");
    m_attackTable = eng::animation::AttackTable::fromFile(
        ASSET_ROOT "/assets/data/player_attacks.json");

    // M7 — Color grade LUT
    {
        auto lut = std::make_shared<eng::render::Texture3D>(
            eng::render::Texture3D::fromHaldPNG(
                ASSET_ROOT "/assets/textures/lut_cinematic.png", 64));
        m_postFx->setLUT(std::move(lut));
        m_postFx->setLUTStrength(0.85f);
    }

    // HUD
    m_hud = std::make_unique<Hud>();
    m_hud->init(wi, hi);

    // Tileset texture
    m_textures.load("tileset",
        ASSET_ROOT "/assets/tilesets/test_tileset.png",
        eng::render::TextureFilter::Nearest);
    m_tilesetTex = m_textures.get("tileset");

    // Parallax background textures
    m_textures.load("parallax_sky",
        ASSET_ROOT "/assets/sprites/parallax_sky.png",
        eng::render::TextureFilter::Linear);
    m_textures.load("parallax_mountains",
        ASSET_ROOT "/assets/sprites/parallax_mountains.png",
        eng::render::TextureFilter::Linear);
    m_textures.load("parallax_trees",
        ASSET_ROOT "/assets/sprites/parallax_trees.png",
        eng::render::TextureFilter::Linear);

    // Parallax layers (sky=furthest, trees=closest)
    // factorX: 0=fixed, 1=scrolls with world. yOffsetFromCam: Y relative to camera center.
    // worldH: must exceed the visible screen height (~8.6m at FOV=30, z=16).
    m_parallax.push_back({m_textures.get("parallax_sky"),       0.05f,  0.5f, 11.f, -4.f, 32.f});
    m_parallax.push_back({m_textures.get("parallax_mountains"), 0.20f, -1.0f,  9.f, -3.f, 32.f});
    m_parallax.push_back({m_textures.get("parallax_trees"),     0.50f, -2.5f,  7.f, -2.f, 32.f});

    // TileMap
    m_tileMap = std::make_unique<eng::map::TileMap>(
        eng::map::TileMap::fromFile(ASSET_ROOT "/maps/test_level.tmx"));

    // Physics debug draw
    m_physics.world().SetDebugDraw(m_dbgDraw);

    // Combat contact listener
    m_physics.setContactListener(&m_contactListener);

    // Camera initial state
    m_camState = sys::CameraState{};

    buildLevel();

    // Start ambient music loop
    eng::audio::AudioEngine::instance().playMusic("ambient_loop.wav");

    LOG_INFO("M7 ready — A/D move | Space jump | J attack | F1 debug | F5 save | F9 load | ESC pause");
}

void GameScene::reinit(eng::core::App& /*app*/) {
    using namespace eng::physics;

    // Clear all Box2D bodies (b2World stays, contact listener stays)
    {
        b2Body* b = m_physics.world().GetBodyList();
        while (b) { b2Body* next = b->GetNext(); m_physics.world().DestroyBody(b); b = next; }
    }

    // Reset ECS and gameplay containers
    m_reg.clear();
    m_enemies.clear();
    m_fixtureData.clear();
    m_player   = eng::ecs::kNullEntity;
    m_miniBoss = eng::ecs::kNullEntity;
    m_camState = sys::CameraState{};
    m_deathTimer        = 0.f;
    m_playerDeadElapsed = false;
    m_hitStopTimer      = 0.f;
    m_timeScale         = 1.f;

    // Load save data (will restore pos/hp)
    auto sd = eng::save::SaveSystem{}.load();
    if (sd) m_saveData = *sd;

    // Re-wire the contact listener (registry was rebuilt)
    m_physics.setContactListener(&m_contactListener);
    m_physics.world().SetDebugDraw(m_dbgDraw);

    // Rebuild tileset + parallax + tilemap + level entities
    m_tilesetTex = m_textures.get("tileset");
    m_tileMap    = std::make_unique<eng::map::TileMap>(
        eng::map::TileMap::fromFile(ASSET_ROOT "/maps/test_level.tmx"));

    m_parallax.clear();
    m_parallax.push_back({m_textures.get("parallax_sky"),       0.05f,  0.5f, 11.f, -4.f, 32.f});
    m_parallax.push_back({m_textures.get("parallax_mountains"), 0.20f, -1.0f,  9.f, -3.f, 32.f});
    m_parallax.push_back({m_textures.get("parallax_trees"),     0.50f, -2.5f,  7.f, -2.f, 32.f});

    buildLevel();

    // Restore player position and HP from save
    if (sd && m_reg.valid(m_player)) {
        if (m_reg.has<RigidBody>(m_player))
            m_reg.get<RigidBody>(m_player).body->SetTransform(toB2(sd->player.pos), 0.f);
        if (m_reg.has<Health>(m_player)) {
            auto& hp      = m_reg.get<Health>(m_player);
            hp.current    = std::min(sd->player.hp, hp.max);
            hp.dead       = false;
            hp.invulnTimer = 0.f;
        }
    }

    if (m_hud) m_hud->setBossVisible(false);
    m_playtime = sd ? sd->playtime : 0.f;

    // Resume music if needed
    eng::audio::AudioEngine::instance().setMusicPaused(false);
}

// Helper: set category/mask filter on every fixture of a body
static void setBodyFilter(b2Body* body, uint16_t cat, uint16_t mask) {
    b2Filter f;
    f.categoryBits = cat;
    f.maskBits     = mask;
    for (auto* fix = body->GetFixtureList(); fix; fix = fix->GetNext())
        fix->SetFilterData(f);
}
// Helper: set category/mask filter on a single fixture
static void setFixtureFilter(b2Fixture* fix, uint16_t cat, uint16_t mask) {
    b2Filter f;
    f.categoryBits = cat;
    f.maskBits     = mask;
    fix->SetFilterData(f);
}

void GameScene::buildLevel() {
    using namespace eng::physics;

    // ── Static world geometry from TMX "collision" object layer ──────────
    glm::vec2 playerSpawnPos{-18.f, 2.f}; // default if not found in TMX

    if (m_tileMap) {
        for (const auto& ol : m_tileMap->objectLayers) {
            if (ol.name == "collision") {
                for (const auto& obj : ol.objects) {
                    if (obj.type != "solid") continue;
                    const glm::vec2 center =
                        kMapOrigin + m_tileMap->tiledToWorld(obj.pos, obj.size);
                    const float halfW = obj.size.x * 0.5f / 32.f;
                    const float halfH = obj.size.y * 0.5f / 32.f;
                    b2Body* wall = m_physics.createStaticBox(center, halfW, halfH);
                    setBodyFilter(wall, kCatWorld, kMaskWorld);
                }
            } else if (ol.name == "spawns") {
                for (const auto& obj : ol.objects) {
                    if (obj.type == "player_spawn") {
                        playerSpawnPos =
                            kMapOrigin + m_tileMap->tiledToWorld(obj.pos, obj.size);
                    }
                }
            }
        }
    } else {
        // Fallback hard-coded geometry (shouldn't happen if TMX loaded)
        b2Body* ground    = m_physics.createStaticBox({0.f, -0.5f},  8.f,  0.5f);
        b2Body* leftWall  = m_physics.createStaticBox({-8.f, 4.f},  0.12f, 5.f);
        b2Body* rightWall = m_physics.createStaticBox({ 8.f, 4.f},  0.12f, 5.f);
        setBodyFilter(ground,    kCatWorld, kMaskWorld);
        setBodyFilter(leftWall,  kCatWorld, kMaskWorld);
        setBodyFilter(rightWall, kCatWorld, kMaskWorld);
    }

    // ── Player ────────────────────────────────────────────────────────────
    m_player = m_reg.create();
    b2Body* playerBody = m_physics.createDynamicBox(
        playerSpawnPos, 0.25f, 0.5f, 1.f, 0.1f);
    playerBody->SetLinearDamping(0.f);
    setBodyFilter(playerBody, kCatPlayer, kMaskPlayer);

    m_reg.emplace<Transform>(m_player);
    m_reg.emplace<RigidBody>(m_player, RigidBody{playerBody, 0.25f, 0.5f});

    auto& psr  = m_reg.emplace<SpriteRenderer>(m_player);
    psr.tex    = m_textures.get("player_sheet");
    psr.size   = {0.5f, 1.0f};

    m_reg.emplace<PlayerControl>(m_player);

    auto& anim = m_reg.emplace<Animator>(m_player);
    anim.clip  = m_clips.get("idle");

    b2Fixture* hitFix = m_physics.addSensorBox(
        playerBody, {0.6f, 0.f}, 0.4f, 0.3f);
    setFixtureFilter(hitFix, kCatPlayerAttack, kMaskPlayerAttack);
    hitFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hitbox, m_player));
    auto& hitbox      = m_reg.emplace<Hitbox>(m_player);
    hitbox.fixture    = hitFix;
    hitbox.damage     = 1.f;
    hitbox.knockback  = {4.f, 1.5f};

    b2Fixture* playerHurtFix = m_physics.addSensorBox(
        playerBody, {0.f, 0.f}, 0.25f, 0.45f);
    setFixtureFilter(playerHurtFix, kCatPlayer, kCatEnemyAttack);
    playerHurtFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hurtbox, m_player));
    auto& playerHurtbox   = m_reg.emplace<Hurtbox>(m_player);
    playerHurtbox.fixture = playerHurtFix;
    playerHurtbox.owner   = m_player;
    m_reg.emplace<Health>(m_player, Health{5.f, 5.f});

    // ── Enemy + boss spawns from TMX "spawns" object layer ───────────────────
    if (m_tileMap) {
        for (const auto& ol : m_tileMap->objectLayers) {
            if (ol.name != "spawns") continue;
            for (const auto& obj : ol.objects) {
                if (obj.type == "enemy_walker" ||
                    obj.type == "enemy_flyer"  ||
                    obj.type == "enemy_ranged") {
                    spawnEnemyFromObject(obj);
                } else if (obj.type == "miniboss") {
                    spawnMiniBossFromObject(obj);
                }
            }
        }
    }

    // ── Camera initial position (follow player spawn) ─────────────────────
    m_camState.pos       = {playerSpawnPos.x, playerSpawnPos.y + 1.5f};
    m_camState.camTarget = m_camState.pos;
}

void GameScene::spawnEnemyFromObject(const eng::map::MapObject& obj) {
    // Resolve world position (center of spawn point)
    const glm::vec2 localPos = m_tileMap->tiledToWorld(obj.pos, obj.size);
    const glm::vec2 worldPos = kMapOrigin + localPos;

    EnemyKind kind{EnemyKind::Walker};
    if      (obj.type == "enemy_flyer")  kind = EnemyKind::Flyer;
    else if (obj.type == "enemy_ranged") kind = EnemyKind::Ranged;

    using namespace eng::physics;

    const eng::ecs::Entity en = m_reg.create();
    m_enemies.push_back(en);

    float halfW = 0.25f, halfH = 0.45f, density = 4.f, friction = 0.2f;
    bool  gravityOff = (kind == EnemyKind::Flyer);

    b2Body* body = m_physics.createDynamicBox(worldPos, halfW, halfH, density, friction);
    body->SetLinearDamping(gravityOff ? 2.f : 0.f);
    body->SetFixedRotation(true);
    if (gravityOff) body->SetGravityScale(0.f);
    setBodyFilter(body, kCatEnemy, kMaskEnemy);

    m_reg.emplace<Transform>(en);
    m_reg.emplace<RigidBody>(en, RigidBody{body, halfW, halfH});

    // Sprite renderer — we use dummy texture as placeholder; Fase G will replace
    const char* texKey = (kind == EnemyKind::Flyer)  ? "enemy_flyer"
                       : (kind == EnemyKind::Ranged) ? "enemy_ranged"
                                                      : "enemy_walker";
    auto& sr  = m_reg.emplace<SpriteRenderer>(en);
    if (m_textures.has(texKey))
        sr.tex = m_textures.get(texKey);
    else
        sr.tex = m_textures.get("dummy"); // fallback until Fase G adds enemy sheets
    sr.size = {halfW * 2.f, halfH * 2.f};

    // Hurtbox
    b2Fixture* hurtFix = m_physics.addSensorBox(body, {0.f, 0.f}, halfW, halfH);
    setFixtureFilter(hurtFix, kCatEnemy, kCatPlayerAttack);
    hurtFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hurtbox, en));
    auto& hb  = m_reg.emplace<Hurtbox>(en);
    hb.fixture = hurtFix;
    hb.owner   = en;

    // Hitbox (attack sensor — offset to the right, facing will flip category later)
    b2Fixture* hitFix = m_physics.addSensorBox(body, {halfW + 0.35f, 0.f}, 0.35f, 0.25f);
    setFixtureFilter(hitFix, kCatEnemyAttack, kMaskEnemyAttack);
    hitFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hitbox, en));
    auto& hx    = m_reg.emplace<Hitbox>(en);
    hx.fixture  = hitFix;
    hx.damage   = 1.f;
    hx.knockback = {3.f, 1.f}; // direction resolved at hit time via attacker facing

    m_reg.emplace<Health>(en, Health{3.f, 3.f});

    // EnemyAI component
    auto& ai       = m_reg.emplace<EnemyAI>(en);
    ai.kind        = kind;
    ai.patrolMinX  = worldPos.x - 3.f;
    ai.patrolMaxX  = worldPos.x + 3.f;
    ai.visionRange = (kind == EnemyKind::Ranged) ? 8.f : 6.f;
    ai.attackRange = (kind == EnemyKind::Ranged) ? 5.f : 1.4f;
    ai.speed       = (kind == EnemyKind::Flyer)  ? 2.f : 2.5f;
}

void GameScene::spawnMiniBossFromObject(const eng::map::MapObject& obj) {
    using namespace eng::physics;

    const glm::vec2 worldPos = kMapOrigin + m_tileMap->tiledToWorld(obj.pos, obj.size);

    m_miniBoss = m_reg.create();

    const float halfW = 0.5f, halfH = 0.8f;
    b2Body* body = m_physics.createDynamicBox(worldPos, halfW, halfH, 8.f, 0.2f);
    body->SetLinearDamping(0.f);
    body->SetFixedRotation(true);
    setBodyFilter(body, kCatEnemy, kMaskEnemy);

    m_reg.emplace<Transform>(m_miniBoss);
    m_reg.emplace<RigidBody>(m_miniBoss, RigidBody{body, halfW, halfH});

    auto& sr  = m_reg.emplace<SpriteRenderer>(m_miniBoss);
    sr.tex    = m_textures.has("boss") ? m_textures.get("boss") : m_textures.get("dummy");
    sr.size   = {halfW * 2.f, halfH * 2.f};

    // Hurtbox
    b2Fixture* hurtFix = m_physics.addSensorBox(body, {0.f, 0.f}, halfW, halfH);
    setFixtureFilter(hurtFix, kCatEnemy, kCatPlayerAttack);
    hurtFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hurtbox, m_miniBoss));
    auto& hb  = m_reg.emplace<Hurtbox>(m_miniBoss);
    hb.fixture = hurtFix;
    hb.owner   = m_miniBoss;

    // Hitbox (melee)
    b2Fixture* hitFix = m_physics.addSensorBox(body, {halfW + 0.5f, 0.f}, 0.5f, 0.4f);
    setFixtureFilter(hitFix, kCatEnemyAttack, kMaskEnemyAttack);
    hitFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hitbox, m_miniBoss));
    auto& hx    = m_reg.emplace<Hitbox>(m_miniBoss);
    hx.fixture  = hitFix;
    hx.damage   = 2.f;
    hx.knockback = {5.f, 2.f}; // direction resolved at hit time via attacker facing

    m_reg.emplace<Health>(m_miniBoss, Health{30.f, 30.f});

    auto& ai       = m_reg.emplace<EnemyAI>(m_miniBoss);
    ai.kind        = EnemyKind::MiniBoss;
    ai.patrolMinX  = worldPos.x - 5.f;
    ai.patrolMaxX  = worldPos.x + 5.f;
    ai.visionRange = 14.f;
    ai.attackRange = 1.8f;
    ai.speed       = 2.f;

    m_reg.emplace<BossPhaseData>(m_miniBoss);
    m_reg.emplace<Animator>(m_miniBoss,
        Animator{m_clips.get("boss_stance_idle")});

    m_hud->setBossVisible(true);
}

eng::ecs::Entity GameScene::spawnProjectile(glm::vec2 from, glm::vec2 vel, float damage) {
    using namespace eng::physics;

    const eng::ecs::Entity en = m_reg.create();

    b2Body* body = m_physics.createDynamicBox(from, 0.12f, 0.08f, 0.1f, 0.f);
    body->SetGravityScale(0.f);
    body->SetLinearDamping(0.f);
    body->SetFixedRotation(true);
    body->SetLinearVelocity(toB2(vel));
    setBodyFilter(body, kCatEnemyAttack, kMaskEnemyAttack);

    m_reg.emplace<Transform>(en);
    m_reg.emplace<RigidBody>(en, RigidBody{body, 0.12f, 0.08f});
    m_reg.emplace<Projectile>(en, Projectile{3.f, damage, false});

    // Hitbox sensor on the projectile body
    b2Fixture* hitFix = m_physics.addSensorBox(body, {0.f, 0.f}, 0.12f, 0.08f);
    setFixtureFilter(hitFix, kCatEnemyAttack, kMaskEnemyAttack);
    hitFix->GetUserData().pointer =
        reinterpret_cast<uintptr_t>(makeFixtureUD(m_fixtureData, sys::FixtureTag::Hitbox, en));
    auto& hx    = m_reg.emplace<Hitbox>(en);
    hx.fixture  = hitFix;
    hx.damage   = damage;
    hx.knockback = {(vel.x > 0 ? 2.f : -2.f), 0.5f};
    hx.active   = true; // always active

    auto& sr  = m_reg.emplace<SpriteRenderer>(en);
    sr.tex    = m_textures.get("dummy"); // placeholder until we have a bullet sprite
    sr.size   = {0.2f, 0.15f};
    sr.tint   = {1.f, 0.4f, 0.1f, 1.f}; // orange-ish tint

    return en;
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

    // Save / Load
    if (input.pressed(eng::input::Action::SaveGame))  doSave();
    if (input.pressed(eng::input::Action::LoadGame))  doLoad();

    // --- Player world position (used by enemy AI) ---
    glm::vec2 playerPos{0.f, 0.f};
    if (m_reg.valid(m_player) && m_reg.has<Transform>(m_player))
        playerPos = glm::vec2{m_reg.get<Transform>(m_player).position};

    // --- Gameplay systems (use scaled dt) ---
    auto& audio = eng::audio::AudioEngine::instance();
    auto onSfx = [&audio](const std::string& sfxName) { audio.playSfx(sfxName); };
    sys::playerControllerUpdate(m_reg, input, m_physics, gdt, m_clips, onSfx);
    sys::animationUpdate(m_reg, gdt);

    // Enemy AI (runs after animation so clip swaps take effect this frame)
    auto spawnProj = [this](glm::vec2 from, glm::vec2 vel, float dmg) {
        return spawnProjectile(from, vel, dmg);
    };
    auto onEnemyDeath = [this](glm::vec3 pos, int kind) {
        m_particleSys->spawn(eng::render::ParticleKind::DeathBurst, pos, kind);
    };
    sys::enemyAIUpdate(m_reg, m_physics, playerPos, gdt, m_clips, spawnProj, onEnemyDeath);

    sys::combatPreUpdate(m_reg, gdt, m_attackTable);
    sys::enemyCombatPreUpdate(m_reg, gdt);
    m_physics.step(gdt);
    sys::physicsSyncUpdate(m_reg);
    sys::projectileUpdate(m_reg, m_physics, m_particleSys.get(), gdt);

    // --- Combat post (resolve hits, apply callbacks) ---
    auto onHitStop  = [this](float dur) { requestHitStop(dur); };
    auto onTrauma   = [this](float amt) { sys::cameraAddTrauma(m_camState, amt); };
    auto onHitSpark = [this](glm::vec3 pos) {
        m_particleSys->spawn(eng::render::ParticleKind::HitSpark, pos);
    };
    auto onDamageDealt = [&audio, this](glm::vec3 pos, float dmg) {
        audio.playSfx("hit");
        if (m_hud) m_hud->pushDamagePopup(pos, dmg);
    };
    sys::combatPostUpdate(m_reg, m_contactListener, onHitStop, onTrauma, onHitSpark, onDamageDealt);

    // --- Player death timer (real dt, not scaled) ---
    if (!m_playerDeadElapsed && m_reg.valid(m_player) && m_reg.has<Health>(m_player)) {
        if (m_reg.get<Health>(m_player).dead) {
            m_deathTimer += dt;
            if (m_deathTimer >= 1.5f) m_playerDeadElapsed = true;
        }
    }

    // --- Camera (uses real dt — continues animating during hit-stop) ---
    if (m_reg.valid(m_player)) {
        const auto& t    = m_reg.get<Transform>(m_player);
        const auto& ctrl = m_reg.get<PlayerControl>(m_player);
        const bool  atk  = (ctrl.state == PlayerState::Attack);
        sys::cameraUpdate(m_camState, t.position, ctrl.facing, atk, dt, m_camera);
    }

    // --- Mini-boss update ---
    if (m_reg.valid(m_miniBoss)) {
        auto spawnProjBoss = [this](glm::vec2 from, glm::vec2 vel, float dmg) {
            return spawnProjectile(from, vel, dmg);
        };
        auto onPhaseChange = [this](BossPhase /*prev*/, BossPhase /*next*/) {
            requestHitStop(0.15f);
            sys::cameraAddTrauma(m_camState, 0.7f);
            eng::audio::AudioEngine::instance().playSfx("hit");
            if (m_hud) m_hud->onBossPhaseChange();
        };
        auto onBossDeath = [this](glm::vec3 pos) {
            m_particleSys->spawn(eng::render::ParticleKind::DeathBurst, pos, 1);
            if (m_hud) m_hud->setBossVisible(false);
            doSave();
        };
        sys::updateMiniBoss(m_miniBoss, m_reg, m_physics, playerPos, gdt,
                            m_bossTable, m_clips, spawnProjBoss,
                            onPhaseChange, onBossDeath);
    }

    // --- HUD update ---
    m_playtime += gdt;
    if (m_hud) m_hud->update(gdt);

    // --- Damage popup wire-up (re-bind each frame; callbacks captured by ref) ---
    // (Note: onDamageDealt above already calls audio; Hud popup is added via the
    //  combatPostUpdate overload that also calls onDamageDealt with worldPos.)

    // --- Particles ---
    m_particleSys->update(dt);

    // Ambient dust: 8 particles/second scattered over the map area
    m_dustTimer += dt;
    constexpr float kDustInterval = 1.f / 8.f;
    while (m_dustTimer >= kDustInterval) {
        m_dustTimer -= kDustInterval;
        const float rx = kMapOrigin.x + static_cast<float>(rand() % 400) / 10.f;
        const float ry = static_cast<float>(rand() % 60) / 10.f; // 0..6m height
        m_particleSys->spawn(eng::render::ParticleKind::Dust,
                             {rx, ry, -1.5f});
    }
}

void GameScene::render() {
    // ── HDR pass: scene → m_postFx's HDR FBO ─────────────────────────────────
    m_postFx->beginScene();

    // Set fog uniforms on the sprite shader once (persist across begin/end cycles)
    m_batch->shader().bind();
    m_batch->shader().set("uFogDensity", m_postFx->fogDensity());
    m_batch->shader().set("uFogColor",   m_postFx->fogColor());

    // 1. Parallax backgrounds
    eng::render::drawParallax(*m_batch, m_camera, m_parallax);

    // 2. Tilemap
    if (m_tileMap && m_tilesetTex && !m_tileMap->tileLayers.empty()) {
        const auto& ts = m_tileMap->tilesets[0];
        m_batch->begin(m_camera);
        eng::map::drawTileLayer(*m_batch, *m_tileMap, m_tileMap->tileLayers[0],
                                *m_tilesetTex, ts, 32.f,
                                kMapOrigin, -0.1f);
        m_batch->end();
    }

    // 3. Entity sprites (player, enemies, projectiles)
    m_batch->begin(m_camera);
    sys::spriteRenderUpdate(m_reg, *m_batch);
    m_batch->end();

    // 4. Particles (rendered inside HDR pass so they feed bloom)
    m_particleSys->render(m_camera);

    // ── Post-process: bloom + god rays + ACES tonemap → default FB ───────────
    m_postFx->endSceneAndComposite(m_camera, kSunWorldPos);

    // ── HUD (screen-space, default FB, after post-process) ───────────────────
    if (m_hud)
        m_hud->render(*m_batch, m_camera, m_reg, m_player, m_miniBoss);

    // ── Overlays (rendered directly to default FB — outside HDR) ─────────────
    if (m_debugDrawOn) {
        m_physics.world().DebugDraw();
        m_dbgDraw->render(m_camera);
    }
}

void GameScene::handleEvent(const SDL_Event& ev) {
    if (ev.type == SDL_WINDOWEVENT &&
        ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        const int   wi = ev.window.data1;
        const int   hi = ev.window.data2;
        m_camera.setAspect(static_cast<float>(wi) / static_cast<float>(hi));
        if (m_postFx) m_postFx->resize(wi, hi);
        if (m_hud)    m_hud->resize(wi, hi);
    }
}

void GameScene::doSave() {
    eng::save::SaveData sd;
    sd.playtime = m_playtime;

    if (m_reg.valid(m_player)) {
        if (m_reg.has<Transform>(m_player)) {
            const auto& t = m_reg.get<Transform>(m_player);
            sd.player.pos = glm::vec2{t.position};
        }
        if (m_reg.has<Health>(m_player)) {
            sd.player.hp = m_reg.get<Health>(m_player).current;
        }
    }

    // Mark defeated enemies (those with Health::dead == true)
    for (auto en : m_enemies) {
        if (!m_reg.valid(en)) continue;
        if (m_reg.has<Health>(en) && m_reg.get<Health>(en).dead)
            sd.defeated.push_back("enemy_" + std::to_string(en.id));
    }
    if (m_reg.valid(m_miniBoss) && m_reg.has<Health>(m_miniBoss)) {
        if (m_reg.get<Health>(m_miniBoss).dead)
            sd.defeated.push_back("miniboss");
    }

    // Carry over existing settings (volume etc.)
    sd.settings = m_saveData.settings;

    eng::save::SaveSystem::save(sd);
    m_saveData = sd;
    LOG_INFO("Saved to {}", eng::save::SaveSystem::savePath());
}

void GameScene::doLoad() {
    auto opt = eng::save::SaveSystem::load();
    if (!opt) {
        LOG_INFO("No save file found.");
        return;
    }
    m_saveData = *opt;
    m_playtime = m_saveData.playtime;

    if (m_reg.valid(m_player)) {
        if (m_reg.has<RigidBody>(m_player)) {
            auto& rb = m_reg.get<RigidBody>(m_player);
            rb.body->SetTransform(
                {m_saveData.player.pos.x, m_saveData.player.pos.y}, 0.f);
            rb.body->SetLinearVelocity({0.f, 0.f});
        }
        if (m_reg.has<Health>(m_player)) {
            m_reg.get<Health>(m_player).current = m_saveData.player.hp;
            m_reg.get<Health>(m_player).dead    = false;
        }
    }

    LOG_INFO("Loaded save (playtime={}s)", m_saveData.playtime);
}
