#pragma once
#include <glm/glm.hpp>

enum class BossPhase { Stance, Enrage, Desperate, Transitioning };

// Boss-specific component (separate from EnemyAI to avoid polluting the shared struct).
struct BossPhaseData {
    BossPhase currentPhase   {BossPhase::Stance};
    BossPhase nextPhase      {BossPhase::Stance};  // set during Transitioning

    // Phase entry HP thresholds (fraction of maxHP)
    float     enrageThreshold   {0.66f};
    float     desperateThreshold{0.33f};

    // Transition state
    float transitionTimer {0.f};  // counts down from kTransitionDuration; >0 = frozen
    float flashIntensity  {0.f};

    // Attack window state (index into current phase's window list)
    int   attackWindowIdx {0};
    float windowTimer     {0.f};  // counts time within the current window phase

    // Dash
    float dashTimer       {0.f};
    bool  dashing         {false};
    glm::vec2 dashVel     {0.f, 0.f};

    // Radial fire
    float radialFireTimer {0.f};
    bool  radialFired     {false};  // prevent multi-fire in same window
};
