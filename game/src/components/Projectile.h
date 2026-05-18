#pragma once
#include <box2d/box2d.h>

struct Projectile {
    float lifetime   {3.f};   // seconds until auto-destroy
    float damage     {1.f};
    bool  fromPlayer {false}; // true = player bullet, false = enemy bullet
};
