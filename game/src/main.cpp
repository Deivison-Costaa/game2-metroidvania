#include "engine/core/App.h"
#include "engine/core/Log.h"
#include "engine/input/Action.h"
#include "engine/audio/AudioEngine.h"
#include "engine/save/SaveSystem.h"
#include "scenes/GameScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneId.h"
#include <stdexcept>
#include <memory>

class GameApp final : public eng::core::App {
public:
    GameApp() : App("Game2 — Metroidvania", 1280, 720) {}

protected:
    void onInit() override {
        eng::audio::AudioEngine::instance().init(ASSET_ROOT "/audio");

        const bool hasSave = eng::save::SaveSystem::load().has_value();

        m_menu = std::make_unique<MenuScene>();
        m_menu->init(1280, 720, std::string(ASSET_ROOT), hasSave);

        m_game = std::make_unique<GameScene>();

        m_current = SceneId::MainMenu;
    }

    void onUpdate(float dt) override {
        using Act = eng::input::Action;
        auto& inp = input();

        if (inp.pressed(Act::Quit)) { quit(); return; }

        switch (m_current) {

        case SceneId::MainMenu: {
            const MenuAction act = m_menu->update(dt, inp);
            switch (act) {
            case MenuAction::NewGame:
                eng::save::SaveSystem::erase();
                m_game->init(*this);
                transitionTo(SceneId::Playing);
                break;
            case MenuAction::Continue:
                if (eng::save::SaveSystem::load().has_value()) {
                    m_game->init(*this);
                    transitionTo(SceneId::Playing);
                }
                break;
            case MenuAction::ToSettings:
                m_prevPage = MenuScene::Page::Main;
                m_menu->setPage(MenuScene::Page::Settings);
                transitionTo(SceneId::Settings);
                break;
            case MenuAction::Quit:
                quit();
                break;
            default: break;
            }
            break;
        }

        case SceneId::Playing: {
            // Pause on Esc
            if (inp.pressed(Act::MenuCancel)) {
                m_menu->setPage(MenuScene::Page::Pause);
                eng::audio::AudioEngine::instance().setMusicPaused(true);
                transitionTo(SceneId::Paused);
                break;
            }

            m_game->update(dt, *this);

            // Check for player death → GameOver after 1.5s delay
            if (m_game->playerDeadDelayElapsed()) {
                m_menu->setPage(MenuScene::Page::GameOver);
                transitionTo(SceneId::GameOver);
            }
            break;
        }

        case SceneId::Paused: {
            const MenuAction act = m_menu->update(dt, inp);
            switch (act) {
            case MenuAction::Resume:
                eng::audio::AudioEngine::instance().setMusicPaused(false);
                transitionTo(SceneId::Playing);
                break;
            case MenuAction::ToSettings:
                m_prevPage = MenuScene::Page::Pause;
                m_menu->setPage(MenuScene::Page::Settings);
                transitionTo(SceneId::Settings);
                break;
            case MenuAction::ToMain:
                eng::audio::AudioEngine::instance().setMusicPaused(false);
                m_menu->setPage(MenuScene::Page::Main);
                m_menu->setHasSave(eng::save::SaveSystem::load().has_value());
                transitionTo(SceneId::MainMenu);
                break;
            case MenuAction::Quit:
                quit();
                break;
            default: break;
            }
            break;
        }

        case SceneId::GameOver: {
            const MenuAction act = m_menu->update(dt, inp);
            switch (act) {
            case MenuAction::Retry:
                m_game->reinit(*this);
                transitionTo(SceneId::Playing);
                break;
            case MenuAction::ToMain:
                m_menu->setPage(MenuScene::Page::Main);
                m_menu->setHasSave(eng::save::SaveSystem::load().has_value());
                transitionTo(SceneId::MainMenu);
                break;
            case MenuAction::Quit:
                quit();
                break;
            default: break;
            }
            break;
        }

        case SceneId::Settings: {
            const MenuAction act = m_menu->update(dt, inp);
            if (act == MenuAction::BackFromSettings) {
                m_menu->setPage(m_prevPage);
                transitionTo(m_prevPage == MenuScene::Page::Main   ? SceneId::MainMenu
                           : m_prevPage == MenuScene::Page::Pause  ? SceneId::Paused
                                                                    : SceneId::MainMenu);
            } else if (act == MenuAction::Quit) {
                quit();
            }
            break;
        }

        } // switch m_current
    }

    void onRender() override {
        switch (m_current) {
        case SceneId::MainMenu:
            m_menu->render(false);
            break;
        case SceneId::Playing:
            m_game->render();
            break;
        case SceneId::Paused:
            m_game->render();
            m_menu->render(true);
            break;
        case SceneId::GameOver:
            m_game->render();
            m_menu->render(true);
            break;
        case SceneId::Settings:
            // Settings may appear over game or over a blank; render game if available
            if (m_prevPage == MenuScene::Page::Pause)
                m_game->render();
            m_menu->render(m_prevPage == MenuScene::Page::Pause);
            break;
        }
    }

    void onEvent(const SDL_Event& ev) override {
        if (m_current == SceneId::Playing && m_game)
            m_game->handleEvent(ev);
    }

    void onShutdown() override {
        eng::audio::AudioEngine::instance().shutdown();
        LOG_INFO("Shutdown clean");
    }

private:
    void transitionTo(SceneId next) { m_current = next; }

    SceneId            m_current  {SceneId::MainMenu};
    MenuScene::Page    m_prevPage {MenuScene::Page::Main};

    std::unique_ptr<GameScene>  m_game;
    std::unique_ptr<MenuScene>  m_menu;
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
