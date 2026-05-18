#include "systems/PhysicsSyncSystem.h"
#include "components/Transform.h"
#include "components/RigidBody.h"

namespace sys {

void physicsSyncUpdate(eng::ecs::Registry& reg) {
    for (auto [e, transform, rb] : reg.view<Transform, RigidBody>()) {
        if (!rb.body) continue;
        const b2Vec2 pos = rb.body->GetPosition();
        transform.position = {pos.x, pos.y, 0.f};
        transform.rotation = rb.body->GetAngle();
    }
}

} // namespace sys
