#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/render/Camera.h"
#include "engine/render/Mesh.h"
#include "engine/render/MeshRenderer.h"
#include "engine/render/SpriteBatch.h"
#include "engine/render/Texture.h"
#include "engine/resources/ResourceManager.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <stdexcept>

// ─── M1 GameApp — 2.5D Renderer Demo ─────────────────────────────────────────
// Renders a low-poly cube (Assimp) + 5 sprites in world-space sharing the same
// perspective camera (FoV 30°). Camera auto-orbits the origin to validate 2.5D.

class GameApp final : public eng::core::App {
public:
    GameApp()
        : App("Game2 — Metroidvania [M1 Renderer]", 1280, 720)
    {}

protected:
    void onInit() override {
        // ── Camera ────────────────────────────────────────────────────────
        const float aspect = static_cast<float>(window().width()) /
                             static_cast<float>(window().height());
        m_camera = std::make_unique<eng::render::Camera>(
            eng::render::Camera::perspective(
                glm::radians(30.f), aspect, 0.1f, 200.f));
        m_camera->setTarget({0.f, 0.f, 0.f});
        m_camera->setUp({0.f, 1.f, 0.f});
        updateCameraOrbit();

        // ── Resources ─────────────────────────────────────────────────────
        m_spriteTex = m_textures.load("knight",
            ASSET_ROOT "/assets/sprites/test.png",
            eng::render::TextureFilter::Nearest);

        m_cubeMesh = std::make_unique<eng::render::Mesh>(
            eng::render::Mesh::fromFile(ASSET_ROOT "/assets/models/cube.obj"));

        // ── Renderers ─────────────────────────────────────────────────────
        m_batch = std::make_unique<eng::render::SpriteBatch>(
            ASSET_ROOT "/shaders/sprite.vert",
            ASSET_ROOT "/shaders/sprite.frag");

        m_meshRenderer = std::make_unique<eng::render::MeshRenderer>(
            ASSET_ROOT "/shaders/mesh.vert",
            ASSET_ROOT "/shaders/mesh.frag");
        m_meshRenderer->setLight(
            {0.6f, 1.f, 0.8f},   // direction
            {1.f, 0.95f, 0.9f},  // color (warm white)
            {0.08f, 0.1f, 0.18f} // ambient (cool blue)
        );

        LOG_INFO("M1 ready — ESC to quit, resize window to test Camera");
    }

    void onUpdate(float dt) override {
        // Auto-orbit: slow rotation around Y axis
        m_orbitAngle += dt * 0.4f;
        updateCameraOrbit();

        // Gentle cube spin
        m_cubeAngle += dt * 0.6f;
    }

    void onRender() override {
        glClearColor(0.04f, 0.03f, 0.06f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ── 3D mesh pass ──────────────────────────────────────────────────
        glm::mat4 model = glm::rotate(glm::mat4(1.f),
            m_cubeAngle, glm::vec3(0.2f, 1.f, 0.3f));
        m_meshRenderer->draw(*m_cubeMesh, *m_camera, model);

        // ── 2D sprite pass (world-space quads, no depth write) ────────────
        m_batch->begin(*m_camera);

        // 5 sprites arranged around the cube
        const std::array<glm::vec3, 5> positions = {{
            { 0.f,  1.8f,  0.f},
            { 1.5f, 0.f,   0.f},
            {-1.5f, 0.f,   0.f},
            { 0.f, -1.8f,  0.f},
            { 0.f,  0.f,   1.5f},
        }};
        const std::array<glm::vec4, 5> tints = {{
            {1.f,  1.f,  1.f,  1.f},
            {1.f,  0.6f, 0.6f, 1.f},
            {0.6f, 0.8f, 1.f,  1.f},
            {0.8f, 1.f,  0.6f, 1.f},
            {1.f,  0.8f, 0.3f, 1.f},
        }};

        for (int i = 0; i < 5; ++i) {
            m_batch->draw(*m_spriteTex,
                positions[static_cast<std::size_t>(i)],
                glm::vec2(0.5f),
                tints[static_cast<std::size_t>(i)]);
        }

        m_batch->end();
    }

    void onEvent(const SDL_Event& ev) override {
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            quit();

        // Keep camera aspect ratio in sync after resize (Window dims already updated in App)
        if (ev.type == SDL_WINDOWEVENT &&
            ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED && m_camera) {
            const float aspect = static_cast<float>(window().width()) /
                                 static_cast<float>(window().height());
            m_camera->setAspect(aspect);
        }
    }

    void onShutdown() override {
        LOG_INFO("Shutdown clean");
    }

private:
    void updateCameraOrbit() {
        if (!m_camera) return;
        const float radius = 8.f;
        const float elevation = glm::radians(12.f); // 12° above horizontal → subtle 2.5D
        m_camera->setPosition({
            radius * glm::cos(elevation) * glm::cos(m_orbitAngle),
            radius * glm::sin(elevation),
            radius * glm::cos(elevation) * glm::sin(m_orbitAngle)
        });
    }

    // Camera
    std::unique_ptr<eng::render::Camera> m_camera;
    float m_orbitAngle{0.f};
    float m_cubeAngle {0.f};

    // Resources
    eng::resources::ResourceManager<eng::render::Texture> m_textures;
    std::shared_ptr<eng::render::Texture>                 m_spriteTex;
    std::unique_ptr<eng::render::Mesh>                    m_cubeMesh;

    // Renderers
    std::unique_ptr<eng::render::SpriteBatch>   m_batch;
    std::unique_ptr<eng::render::MeshRenderer>  m_meshRenderer;
};

// ─── Entry point ─────────────────────────────────────────────────────────────

int main() {
    try {
        GameApp app;
        app.run();
    } catch (const std::exception& ex) {
        LOG_ERROR("Fatal: {}", ex.what());
        return 1;
    }
    return 0;
}
