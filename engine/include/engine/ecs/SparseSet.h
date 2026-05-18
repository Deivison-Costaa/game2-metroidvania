#pragma once
#include <cstdint>
#include <vector>
#include <utility>

namespace eng::ecs {

// Sparse-set storage: O(1) insert/erase/contains, contiguous iteration.
template<typename T>
class SparseSet {
public:
    static constexpr uint32_t kNone = ~0u;

    bool contains(uint32_t id) const noexcept {
        return id < m_sparse.size() && m_sparse[id] != kNone;
    }

    T& insert(uint32_t id, T value) {
        if (id >= m_sparse.size())
            m_sparse.resize(static_cast<size_t>(id) + 1, kNone);
        if (contains(id))
            return m_components[m_sparse[id]] = std::move(value);
        m_sparse[id] = static_cast<uint32_t>(m_dense.size());
        m_dense.push_back(id);
        m_components.push_back(std::move(value));
        return m_components.back();
    }

    void erase(uint32_t id) noexcept {
        if (!contains(id)) return;
        const uint32_t idx  = m_sparse[id];
        const uint32_t last = static_cast<uint32_t>(m_dense.size()) - 1u;
        if (idx != last) {
            m_dense[idx]             = m_dense[last];
            m_components[idx]        = std::move(m_components[last]);
            m_sparse[m_dense[idx]]   = idx;
        }
        m_sparse[id] = kNone;
        m_dense.pop_back();
        m_components.pop_back();
    }

    T* get(uint32_t id) noexcept {
        return contains(id) ? &m_components[m_sparse[id]] : nullptr;
    }
    const T* get(uint32_t id) const noexcept {
        return contains(id) ? &m_components[m_sparse[id]] : nullptr;
    }

    size_t size()  const noexcept { return m_dense.size(); }
    bool   empty() const noexcept { return m_dense.empty(); }

    // Direct access for view iteration
    const std::vector<uint32_t>& dense()      const noexcept { return m_dense; }
    std::vector<T>&               components()       noexcept { return m_components; }

private:
    std::vector<uint32_t> m_sparse;
    std::vector<uint32_t> m_dense;
    std::vector<T>        m_components;
};

} // namespace eng::ecs
