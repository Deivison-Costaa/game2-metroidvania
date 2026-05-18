#pragma once
#include "engine/core/Window.h"
#include <SDL2/SDL.h>
#include <memory>
#include <string_view>

namespace eng::core {

class App {
public:
    explicit App(std::string_view title = "Game2 — Metroidvania",
                 int width  = 1280,
                 int height = 720);
    virtual ~App() = default;

    App(const App&)            = delete;
    App& operator=(const App&) = delete;

    void run();
    void quit() { m_running = false; }

    Window& window()    { return *m_window; }
    float   time() const { return m_time; }

protected:
    virtual void onInit()                      {}
    virtual void onUpdate(float dt)            { (void)dt; }
    virtual void onRender()                    {}
    virtual void onEvent(const SDL_Event& ev)  { (void)ev; }
    virtual void onShutdown()                  {}

private:
    std::unique_ptr<Window> m_window;
    bool  m_running{false};
    float m_time   {0.f};
};

} // namespace eng::core
