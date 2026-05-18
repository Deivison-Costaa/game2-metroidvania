#pragma once

struct PlayerControl {
    float moveSpeed  {6.f};   // m/s horizontal
    float jumpImpulse{9.f};   // applied as velocity impulse (m/s)
    bool  grounded   {false};
};
