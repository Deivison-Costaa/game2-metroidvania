#pragma once
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace eng::animation {

struct AttackWindow {
    float     start{0.f};
    float     end{0.25f};
    glm::vec2 offset{0.6f, 0.f};
    glm::vec2 halfSize{0.4f, 0.3f};
    float     damage{1.f};
    glm::vec2 knockback{4.f, 1.5f};
};

struct AttackData {
    float                    duration{0.25f};
    std::vector<AttackWindow> windows;
};

// Data-driven attack configuration loaded from JSON.
// JSON format: { "attack1": { "duration": 0.25, "windows": [...] } }
class AttackTable {
public:
    static AttackTable fromFile(std::string_view path);

    const AttackData* get(std::string_view name) const;
    bool              has(std::string_view name) const;

private:
    std::unordered_map<std::string, AttackData> m_data;
};

} // namespace eng::animation
