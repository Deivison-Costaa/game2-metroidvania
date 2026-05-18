#pragma once
#include "engine/input/Action.h"
#include <SDL2/SDL.h>
#include <array>
#include <cstddef>

namespace eng::input {

class InputManager {
public:
    InputManager();

    // Call once at the start of each frame (before SDL_PollEvent loop).
    void beginFrame();

    // Feed each SDL event from the poll loop.
    void handleEvent(const SDL_Event& ev);

    // Rebind an action to a different scancode.
    void bind(Action a, SDL_Scancode sc);

    // True while the key is held this frame.
    bool down(Action a) const noexcept;

    // True only on the frame the key was first pressed.
    bool pressed(Action a) const noexcept;

    // True only on the frame the key was released.
    bool released(Action a) const noexcept;

    // Returns -1, 0, or +1 based on neg/pos actions.
    float axis(Action neg, Action pos) const noexcept;

private:
    static constexpr std::size_t kCount = static_cast<std::size_t>(Action::kCount);

    std::array<SDL_Scancode, kCount> m_bindings;
    std::array<bool,         kCount> m_curr{};
    std::array<bool,         kCount> m_prev{};
};

} // namespace eng::input
