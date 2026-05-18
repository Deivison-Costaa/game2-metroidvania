#pragma once
#include <cstddef>

namespace eng::input {

enum class Action : std::size_t {
    MoveLeft,
    MoveRight,
    Jump,
    Attack,
    Dash,
    ToggleDebug,
    Quit,
    kCount
};

} // namespace eng::input
