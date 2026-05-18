#include "engine/physics/PhysicsWorld.h"
#include "engine/physics/PhysicsConstants.h"
#include <algorithm>

namespace eng::physics {

PhysicsWorld::PhysicsWorld(glm::vec2 gravity)
    : m_world(b2Vec2{gravity.x, gravity.y})
{
    m_world.SetAllowSleeping(true);
    m_world.SetContinuousPhysics(true);
}

void PhysicsWorld::step(float dt) {
    m_accumulator += dt;
    const int steps = std::min(static_cast<int>(m_accumulator / kStep), kMaxSteps);
    for (int i = 0; i < steps; ++i) {
        m_world.Step(kStep, 8, 3);
        m_accumulator -= kStep;
    }
}

b2Body* PhysicsWorld::createStaticBox(glm::vec2 pos, float halfW, float halfH,
                                      float friction) {
    b2BodyDef bd;
    bd.type     = b2_staticBody;
    bd.position = toB2(pos);
    b2Body* body = m_world.CreateBody(&bd);

    b2PolygonShape shape;
    shape.SetAsBox(halfW, halfH);

    b2FixtureDef fd;
    fd.shape    = &shape;
    fd.friction = friction;
    body->CreateFixture(&fd);
    return body;
}

b2Body* PhysicsWorld::createDynamicBox(glm::vec2 pos, float halfW, float halfH,
                                       float density, float friction) {
    b2BodyDef bd;
    bd.type            = b2_dynamicBody;
    bd.position        = toB2(pos);
    bd.fixedRotation   = true; // prevents toppling
    b2Body* body = m_world.CreateBody(&bd);

    b2PolygonShape shape;
    shape.SetAsBox(halfW, halfH);

    b2FixtureDef fd;
    fd.shape    = &shape;
    fd.density  = density;
    fd.friction = friction;
    body->CreateFixture(&fd);
    return body;
}

b2Fixture* PhysicsWorld::addSensorBox(b2Body* body, glm::vec2 offset,
                                      float halfW, float halfH) {
    b2PolygonShape shape;
    shape.SetAsBox(halfW, halfH, toB2(offset), 0.f);

    b2FixtureDef fd;
    fd.shape    = &shape;
    fd.isSensor = true;
    return body->CreateFixture(&fd);
}

bool PhysicsWorld::isOnGround(b2Body* body, float halfH, float eps) const {
    struct Callback : b2RayCastCallback {
        b2Body* skip;
        bool hit{false};
        float ReportFixture(b2Fixture* f, const b2Vec2&,
                            const b2Vec2&, float) override {
            if (f->IsSensor() || f->GetBody() == skip) return -1.f;
            hit = true;
            return 0.f;
        }
    } cb;
    cb.skip = body;

    const b2Vec2 from = body->GetPosition();
    const b2Vec2 to   = {from.x, from.y - (halfH + eps)};
    m_world.RayCast(&cb, from, to);
    return cb.hit;
}

void PhysicsWorld::setContactListener(b2ContactListener* listener) {
    m_world.SetContactListener(listener);
}

} // namespace eng::physics
