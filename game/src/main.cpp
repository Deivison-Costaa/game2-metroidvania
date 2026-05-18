#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/render/Shader.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <memory>
#include <stdexcept>

// ─── M0 Bootstrap App ────────────────────────────────────────────────────────
// Draws an animated gradient fullscreen triangle to validate the shader
// pipeline. No VBO needed — vertex positions are generated in the vertex shader
// from gl_VertexID.
// Replace this class with the real GameApp in M1.

class BootstrapApp final : public eng::core::App {
public:
    BootstrapApp()
        : App("Game2 — Metroidvania [M0 Bootstrap]", 1280, 720)
    {}

protected:
    void onInit() override {
        // Load gradient shader from source directory via ASSET_ROOT macro
        m_shader = std::make_unique<eng::render::Shader>(
            eng::render::Shader::fromFiles(
                ASSET_ROOT "/shaders/clear.vert",
                ASSET_ROOT "/shaders/clear.frag"
            )
        );

        // Empty VAO required by OpenGL 4.5 core profile for attribute-less draws
        glGenVertexArrays(1, &m_vao);

        LOG_INFO("M0 bootstrap ready — press ESC to quit");
    }

    void onRender() override {
        glClearColor(0.01f, 0.01f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader->bind();
        m_shader->set("iTime",       time());
        m_shader->set("iResolution", glm::vec2(
            static_cast<float>(window().width()),
            static_cast<float>(window().height())
        ));

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }

    void onEvent(const SDL_Event& ev) override {
        if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE)
            quit();
    }

    void onShutdown() override {
        glDeleteVertexArrays(1, &m_vao);
        LOG_INFO("Shutdown clean");
    }

private:
    std::unique_ptr<eng::render::Shader> m_shader;
    unsigned int m_vao{0};
};

// ─── Entry point ─────────────────────────────────────────────────────────────

int main() {
    try {
        BootstrapApp app;
        app.run();
    } catch (const std::exception& ex) {
        LOG_ERROR("Fatal: {}", ex.what());
        return 1;
    }
    return 0;
}
