#pragma once
#include <box2d/box2d.h>

struct RigidBody {
    b2Body*    body{nullptr};
    float      halfW{0.25f};  // meters
    float      halfH{0.5f};   // meters
};
