#pragma once
#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace eng::resources {

// Cache of shared_ptr<T> keyed by string. T must expose a static fromFile(args...) factory.
template<typename T>
class ResourceManager {
public:
    // Load and cache a resource. Extra args are forwarded to T::fromFile(args...).
    // Returns the cached instance if the key already exists.
    template<typename... Args>
    std::shared_ptr<T> load(const std::string& key, Args&&... args) {
        if (auto it = m_cache.find(key); it != m_cache.end())
            return it->second;
        // Two-step alloc: fromFile returns T by value (move), shared_ptr wraps it.
        auto res = std::shared_ptr<T>(new T(T::fromFile(std::forward<Args>(args)...)));
        m_cache.emplace(key, res);
        return res;
    }

    std::shared_ptr<T> get(const std::string& key) const {
        auto it = m_cache.find(key);
        if (it == m_cache.end())
            throw std::runtime_error(
                std::format("ResourceManager::get — key '{}' not loaded", key));
        return it->second;
    }

    bool has(const std::string& key) const { return m_cache.count(key) > 0; }
    void clear()                           { m_cache.clear(); }

private:
    std::unordered_map<std::string, std::shared_ptr<T>> m_cache;
};

} // namespace eng::resources
