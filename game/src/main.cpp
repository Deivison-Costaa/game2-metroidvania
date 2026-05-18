#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Action.h"
#include "scenes/GameScene.h"
#include <stdexcept>

// ─── M2 GameApp — ECS + Physics + Input demo ─────────────────────────────────
// Side-view 2.5D platformer: player walks on ground with Box2D physics.
// Controls: A/D move | Space jump | F1 toggle debug-draw | ESC quit

class GameApp final : public eng::core::App {
public:
    GameApp() : App("Game2 — Metroidvania [M2 ECS+Physics+Input]", 1280, 720) {}

protected:
    void onInit() override {
        m_scene.init(*this);
    }

    void onUpdate(float dt) override {
        if (input().pressed(eng::input::Action::Quit)) quit();
        m_scene.update(dt, *this);
    }

    void onRender() override {
        m_scene.render();
    }

    void onEvent(const SDL_Event& ev) override {
        m_scene.handleEvent(ev);
    }

    void onShutdown() override {
        LOG_INFO("Shutdown clean");
    }

private:
    GameScene m_scene;
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
