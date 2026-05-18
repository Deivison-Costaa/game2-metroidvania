#include "engine/input/InputManager.h"

namespace eng::input {

InputManager::InputManager() {
    // Default bindings
    m_bindings[static_cast<std::size_t>(Action::MoveLeft)]    = SDL_SCANCODE_A;
    m_bindings[static_cast<std::size_t>(Action::MoveRight)]   = SDL_SCANCODE_D;
    m_bindings[static_cast<std::size_t>(Action::Jump)]        = SDL_SCANCODE_SPACE;
    m_bindings[static_cast<std::size_t>(Action::Attack)]      = SDL_SCANCODE_J;
    m_bindings[static_cast<std::size_t>(Action::Dash)]        = SDL_SCANCODE_K;
    m_bindings[static_cast<std::size_t>(Action::ToggleDebug)] = SDL_SCANCODE_F1;
    m_bindings[static_cast<std::size_t>(Action::Quit)]        = SDL_SCANCODE_ESCAPE;
}

void InputManager::beginFrame() {
    m_prev = m_curr;
    // Snapshot the keyboard state
    const Uint8* ks = SDL_GetKeyboardState(nullptr);
    for (std::size_t i = 0; i < kCount; ++i)
        m_curr[i] = ks[m_bindings[i]] != 0;
}

void InputManager::handleEvent(const SDL_Event& ev) {
    // Left-arrow / right-arrow as alternate Move bindings
    if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
        const bool pressed = (ev.type == SDL_KEYDOWN);
        if (ev.key.keysym.scancode == SDL_SCANCODE_LEFT)
            m_curr[static_cast<std::size_t>(Action::MoveLeft)]  = pressed;
        if (ev.key.keysym.scancode == SDL_SCANCODE_RIGHT)
            m_curr[static_cast<std::size_t>(Action::MoveRight)] = pressed;
    }
    (void)ev; // other events handled by beginFrame snapshot
}

void InputManager::bind(Action a, SDL_Scancode sc) {
    m_bindings[static_cast<std::size_t>(a)] = sc;
}

bool InputManager::down(Action a) const noexcept {
    return m_curr[static_cast<std::size_t>(a)];
}

bool InputManager::pressed(Action a) const noexcept {
    const auto i = static_cast<std::size_t>(a);
    return m_curr[i] && !m_prev[i];
}

bool InputManager::released(Action a) const noexcept {
    const auto i = static_cast<std::size_t>(a);
    return !m_curr[i] && m_prev[i];
}

float InputManager::axis(Action neg, Action pos) const noexcept {
    return (down(pos) ? 1.f : 0.f) - (down(neg) ? 1.f : 0.f);
}

} // namespace eng::input
