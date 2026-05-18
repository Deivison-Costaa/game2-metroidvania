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
    SaveGame,
    LoadGame,
    Quit,
    // ── Menu navigation ──────────────────────────────────────────────────────
    MenuUp,
    MenuDown,
    MenuLeft,
    MenuRight,
    MenuConfirm,
    MenuCancel,
    kCount
};

} // namespace eng::input
