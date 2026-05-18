#include "engine/core/App.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <chrono>
#include <algorithm>
#include <format>

namespace eng::core {

App::App(std::string_view title, int width, int height)
    : m_window(std::make_unique<Window>(title, width, height))
{}

void App::run() {
    onInit();
    m_running = true;

    using Clock = std::chrono::high_resolution_clock;
    auto prev = Clock::now();
    double fpsAccum  = 0.0;
    int    fpsFrames = 0;

    while (m_running) {
        auto  now = Clock::now();
        float dt  = std::chrono::duration<float>(now - prev).count();
        prev = now;
        // Clamp to prevent spiral-of-death on breakpoints / lag spikes
        dt = std::min(dt, 0.05f);
        m_time += dt;

        // ── Event pump ────────────────────────────────────────────────────
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) m_running = false;
            onEvent(ev);
        }

        // ── Update & render ───────────────────────────────────────────────
        onUpdate(dt);
        onRender();
        m_window->swap();

        // ── FPS counter (debug) ───────────────────────────────────────────
        ++fpsFrames;
        fpsAccum += static_cast<double>(dt);
        if (fpsAccum >= 1.0) {
            LOG_INFO("FPS: {:3d}  |  frame: {:.2f} ms",
                fpsFrames,
                (fpsAccum / static_cast<double>(fpsFrames)) * 1000.0);
            fpsFrames = 0;
            fpsAccum  = 0.0;
        }

#ifndef NDEBUG
        // GL error sentinel — catches mis-use during development
        GLenum glErr = glGetError();
        if (glErr != GL_NO_ERROR) {
            LOG_WARN("GL error 0x{:04X} detected after frame", glErr);
        }
#endif
    }

    onShutdown();
}

} // namespace eng::core
