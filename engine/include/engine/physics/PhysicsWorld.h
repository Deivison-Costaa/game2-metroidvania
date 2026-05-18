#pragma once
#include <box2d/box2d.h>
#include <glm/glm.hpp>

namespace eng::physics {

// Wrapper around b2World with a fixed-step accumulator (60 Hz).
// All positions are in meters. Use PhysicsConstants.h for conversions.
class PhysicsWorld {
public:
    explicit PhysicsWorld(glm::vec2 gravity = {0.f, -9.81f});
    ~PhysicsWorld() = default;

    PhysicsWorld(const PhysicsWorld&)            = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    // Step the simulation. Uses a fixed 60 Hz sub-step with an accumulator.
    void step(float dt);

    // Create a static rectangular body (immovable).
    // pos: center in meters, halfW/halfH: half-extents in meters.
    b2Body* createStaticBox(glm::vec2 pos, float halfW, float halfH,
                            float friction = 0.5f);

    // Create a dynamic rectangular body (affected by forces/gravity).
    b2Body* createDynamicBox(glm::vec2 pos, float halfW, float halfH,
                             float density  = 1.f,
                             float friction = 0.3f);

    // Add a sensor fixture to an existing body (no collision response).
    b2Fixture* addSensorBox(b2Body* body, glm::vec2 offset,
                            float halfW, float halfH);

    // True if any non-sensor fixture is touching the ground below the body.
    // Casts a short ray from center downward by (halfH + eps).
    bool isOnGround(b2Body* body, float halfH, float eps = 0.05f) const;

    // Register an external contact listener (replaces any previous one).
    void setContactListener(b2ContactListener* listener);

    b2World& world() noexcept { return m_world; }

private:
    b2World m_world;
    float   m_accumulator{0.f};

    static constexpr float kStep     = 1.f / 60.f;
    static constexpr int   kMaxSteps = 5;
};

} // namespace eng::physics
