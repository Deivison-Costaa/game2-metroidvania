#include "engine/ecs/Registry.h"

namespace eng::ecs {

Entity Registry::create() {
    if (!m_freeList.empty()) {
        const uint32_t id = m_freeList.back();
        m_freeList.pop_back();
        return Entity{id, m_generations[id]};
    }
    const uint32_t id = static_cast<uint32_t>(m_generations.size());
    m_generations.push_back(0u);
    return Entity{id, 0u};
}

void Registry::destroy(Entity e) {
    if (!valid(e)) return;
    for (auto& [_, pool] : m_pools)
        pool->erase(e.id);
    ++m_generations[e.id];
    m_freeList.push_back(e.id);
}

bool Registry::valid(Entity e) const noexcept {
    return e.id < m_generations.size() && m_generations[e.id] == e.gen;
}

} // namespace eng::ecs
