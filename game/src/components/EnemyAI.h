#pragma once
#include <glm/glm.hpp>

enum class EnemyKind  { Walker, Flyer, Ranged, MiniBoss };
enum class EnemyState { Patrol, Chase, Attack, Hurt, Dead };

struct EnemyAI {
    EnemyKind  kind          {EnemyKind::Walker};
    EnemyState state         {EnemyState::Patrol};
    float      stateTimer    {0.f};
    float      attackCooldown{0.f};
    int        facing        {1};      // +1 = right, -1 = left

    float patrolMinX   {-1.f};
    float patrolMaxX   { 1.f};
    float visionRange  { 6.f};  // meters
    float attackRange  { 1.2f}; // meters — switches to Attack state
    float speed        { 2.5f}; // m/s patrol speed

    bool  hasLoS       {false}; // line-of-sight to player (updated each frame)
    float sinePhase    {0.f};   // for Flyer sinusoidal motion

    // For Ranged: fire timer
    float fireTimer    {0.f};
    float fireCooldown {2.5f};
};
