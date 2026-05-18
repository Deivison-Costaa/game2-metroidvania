#pragma once

enum class PlayerState { Idle, Run, Jump, Fall, Attack, Hurt };

struct PlayerControl {
    float moveSpeed    {6.f};   // m/s horizontal
    float jumpImpulse  {9.f};   // vertical velocity on jump (m/s)
    float coyoteTime   {0.10f}; // seconds after leaving ground that jump is still allowed
    float jumpBufferTime{0.15f};// seconds a jump press is buffered before landing
    float attackDuration{0.25f};// seconds the attack state lasts

    bool  grounded     {false};
    int   facing       {+1};    // +1 = right, -1 = left

    PlayerState state     {PlayerState::Idle};
    float       stateTime {0.f};
    float       coyoteTimer    {0.f};
    float       jumpBufferTimer{0.f};
    float       attackTimer    {0.f};

    bool wasGrounded{false}; // ground state from previous frame (for coyote detection)
};
