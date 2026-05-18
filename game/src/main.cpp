#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Action.h"
#include "engine/audio/AudioEngine.h"
#include "scenes/GameScene.h"
#include <stdexcept>

class GameApp final : public eng::core::App {
public:
    GameApp() : App("Game2 — Metroidvania", 1280, 720) {}

protected:
    void onInit() override {
        // Audio must initialise before the scene (scene will call playMusic)
        eng::audio::AudioEngine::instance().init(ASSET_ROOT "/audio");
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
        eng::audio::AudioEngine::instance().shutdown();
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
