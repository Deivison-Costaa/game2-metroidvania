#include "systems/ProjectileSystem.h"
#include "components/Projectile.h"
#include "components/RigidBody.h"
#include "components/Transform.h"
#include <vector>

namespace sys {

void projectileUpdate(eng::ecs::Registry& reg,
                      eng::physics::PhysicsWorld& physics,
                      eng::render::ParticleSystem* particles,
                      float dt)
{
    std::vector<eng::ecs::Entity> toDestroy;

    for (auto [e, proj] : reg.view<Projectile>()) {
        // Emit trail from current position
        if (particles && reg.has<Transform>(e)) {
            particles->spawn(eng::render::ParticleKind::ProjectileTrail,
                             reg.get<Transform>(e).position);
        }

        proj.lifetime -= dt;
        if (proj.lifetime <= 0.f)
            toDestroy.push_back(e);
    }

    for (eng::ecs::Entity e : toDestroy) {
        if (reg.has<RigidBody>(e))
            physics.world().DestroyBody(reg.get<RigidBody>(e).body);
        reg.destroy(e);
    }
}

} // namespace sys
