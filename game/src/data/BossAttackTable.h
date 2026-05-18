#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <optional>

namespace data {

struct AttackHitbox {
    glm::vec2 offset{0.6f, 0.f};
    glm::vec2 size  {0.9f, 0.5f};
    float     damage{2.f};
    glm::vec2 knockback{5.f, 2.f};
};

struct RadialData {
    int   count{8};
    float speed{7.f};
    float damage{1.f};
};

struct AttackWindow {
    std::string               clip;
    float                     windup;   // seconds before active phase
    float                     active;   // seconds the hitbox is live
    float                     recovery; // seconds after active phase
    std::optional<AttackHitbox> hitbox;
    std::optional<glm::vec2>    dashVel; // optional dash impulse during active
    std::optional<RadialData>   radial;  // optional radial fire during active
};

struct PhaseData {
    std::string                name;
    float                      hpEnter;    // fraction of maxHP to enter this phase
    float                      speedMul{1.f};
    std::vector<AttackWindow>  windows;
};

struct BossAttackTable {
    std::vector<PhaseData> phases;

    // Load from JSON (assets/data/boss_attacks.json).
    // Returns a default table if the file cannot be opened.
    static BossAttackTable fromFile(const std::string& path);

    const PhaseData* phaseByFraction(float hpFraction) const;
};

} // namespace data
