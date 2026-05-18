#pragma once

struct Health {
    float current    {3.f};
    float max        {3.f};
    float invulnTimer{0.f};  // seconds of invulnerability after a hit
    float flashTimer {0.f};  // drives the hit-flash visual on SpriteRenderer.tint
    bool  dead       {false};
};
