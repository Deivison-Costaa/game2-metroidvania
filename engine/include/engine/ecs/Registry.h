#pragma once
#include "engine/ecs/Entity.h"
#include "engine/ecs/SparseSet.h"
#include <cassert>
#include <memory>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace eng::ecs {

class Registry {
public:
    Registry()  = default;
    ~Registry() = default;
    Registry(const Registry&)            = delete;
    Registry& operator=(const Registry&) = delete;

    // ── Entity lifecycle ──────────────────────────────────────────────────────

    Entity create();
    void   destroy(Entity e);
    bool   valid(Entity e) const noexcept;

    // Destroy all entities and clear all component pools.
    void clear() { m_pools.clear(); m_generations.clear(); m_freeList.clear(); }

    // ── Component operations ──────────────────────────────────────────────────

    template<typename C, typename... Args>
    C& emplace(Entity e, Args&&... args) {
        assert(valid(e) && "emplace on invalid entity");
        return pool<C>().data.insert(e.id, C{std::forward<Args>(args)...});
    }

    template<typename C>
    void remove(Entity e) {
        auto* p = poolPtr<C>();
        if (p) p->erase(e.id);
    }

    template<typename C>
    C& get(Entity e) {
        C* c = try_get<C>(e);
        assert(c && "Component not found");
        return *c;
    }

    template<typename C>
    const C& get(Entity e) const {
        const C* c = try_get<C>(e);
        assert(c && "Component not found");
        return *c;
    }

    template<typename C>
    C* try_get(Entity e) noexcept {
        if (!valid(e)) return nullptr;
        auto* p = poolPtr<C>();
        return p ? p->get(e.id) : nullptr;
    }

    template<typename C>
    const C* try_get(Entity e) const noexcept {
        if (!valid(e)) return nullptr;
        auto key = std::type_index(typeid(C));
        auto it  = m_pools.find(key);
        if (it == m_pools.end()) return nullptr;
        return static_cast<const Pool<C>&>(*it->second).data.get(e.id);
    }

    template<typename C>
    bool has(Entity e) const noexcept {
        auto key = std::type_index(typeid(C));
        auto it  = m_pools.find(key);
        return it != m_pools.end() &&
               static_cast<const Pool<C>&>(*it->second).data.contains(e.id);
    }

    // ── View ─────────────────────────────────────────────────────────────────
    // Iterates entities that have all listed components.
    // Usage:  for (auto [e, t, r] : reg.view<Transform, RigidBody>()) { ... }

    template<typename C0, typename... Cs>
    struct View {
        using Pivot = C0;

        Registry*                    reg;
        const std::vector<uint32_t>* dense;
        size_t                       sz;

        struct Iterator {
            Registry*                    reg;
            const std::vector<uint32_t>* dense;
            size_t idx;
            size_t end;

            void advance() {
                while (idx < end) {
                    uint32_t id = (*dense)[idx];
                    Entity e{id, reg->m_generations[id]};
                    if (reg->valid(e) && (reg->has<Cs>(e) && ...)) return;
                    ++idx;
                }
            }

            auto operator*() {
                uint32_t id = (*dense)[idx];
                Entity e{id, reg->m_generations[id]};
                return std::tuple<Entity, C0&, Cs&...>{
                    e, reg->get<C0>(e), reg->get<Cs>(e)...};
            }
            Iterator& operator++() { ++idx; advance(); return *this; }
            bool operator!=(const Iterator& o) const { return idx != o.idx; }
        };

        Iterator begin() {
            if (!dense || sz == 0) return {reg, dense, sz, sz};
            Iterator it{reg, dense, 0, sz};
            it.advance();
            return it;
        }
        Iterator end() { return {reg, dense, sz, sz}; }
    };

    template<typename C0, typename... Cs>
    View<C0, Cs...> view() {
        auto* p = poolPtr<C0>();
        if (!p || p->empty())
            return {this, nullptr, 0};
        return {this, &p->dense(), p->size()};
    }

private:
    // ── Type-erased pool ─────────────────────────────────────────────────────

    struct IPool {
        virtual ~IPool() = default;
        virtual void erase(uint32_t id) = 0;
    };

    template<typename C>
    struct Pool : IPool {
        SparseSet<C> data;
        void erase(uint32_t id) override { data.erase(id); }
    };

    template<typename C>
    Pool<C>& pool() {
        auto key = std::type_index(typeid(C));
        auto [it, inserted] = m_pools.emplace(key, nullptr);
        if (inserted) it->second = std::make_unique<Pool<C>>();
        return static_cast<Pool<C>&>(*it->second);
    }

    template<typename C>
    SparseSet<C>* poolPtr() noexcept {
        auto key = std::type_index(typeid(C));
        auto it  = m_pools.find(key);
        return (it != m_pools.end())
            ? &static_cast<Pool<C>&>(*it->second).data
            : nullptr;
    }

    // ── Entity state ──────────────────────────────────────────────────────────
    std::vector<uint32_t>                                       m_generations;
    std::vector<uint32_t>                                       m_freeList;
    std::unordered_map<std::type_index, std::unique_ptr<IPool>> m_pools;
};

} // namespace eng::ecs
