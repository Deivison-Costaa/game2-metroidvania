#include "data/BossAttackTable.h"
#include "engine/core/Log.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace data {

// ── Default table (used when JSON is missing) ─────────────────────────────────

static BossAttackTable makeDefault() {
    BossAttackTable t;

    PhaseData stance;
    stance.name    = "stance";
    stance.hpEnter = 1.00f;
    stance.speedMul= 1.0f;
    stance.windows.push_back({
        "boss_stance_slash", 0.35f, 0.18f, 0.45f,
        AttackHitbox{{0.7f,0.f},{0.8f,0.4f},2.f,{4.f,1.5f}},
        std::nullopt, std::nullopt
    });
    t.phases.push_back(std::move(stance));

    PhaseData enrage;
    enrage.name    = "enrage";
    enrage.hpEnter = 0.66f;
    enrage.speedMul= 1.5f;
    enrage.windows.push_back({
        "boss_enrage_dash", 0.20f, 0.30f, 0.25f,
        std::nullopt, glm::vec2{10.f, 0.f}, std::nullopt
    });
    enrage.windows.push_back({
        "boss_enrage_slash", 0.18f, 0.15f, 0.30f,
        AttackHitbox{{0.7f,0.f},{0.8f,0.4f},2.f,{5.f,2.f}},
        std::nullopt, std::nullopt
    });
    t.phases.push_back(std::move(enrage));

    PhaseData desperate;
    desperate.name    = "desperate";
    desperate.hpEnter = 0.33f;
    desperate.speedMul= 1.5f;
    desperate.windows.push_back({
        "boss_desperate_radial", 0.25f, 0.05f, 0.50f,
        std::nullopt, std::nullopt, RadialData{8, 7.f, 1.f}
    });
    desperate.windows.push_back({
        "boss_enrage_slash", 0.18f, 0.15f, 0.30f,
        AttackHitbox{{0.7f,0.f},{0.8f,0.4f},3.f,{5.f,2.f}},
        std::nullopt, std::nullopt
    });
    t.phases.push_back(std::move(desperate));

    return t;
}

// ── JSON loader ───────────────────────────────────────────────────────────────

BossAttackTable BossAttackTable::fromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        LOG_WARN("BossAttackTable: '{}' not found, using defaults", path);
        return makeDefault();
    }

    try {
        auto j = nlohmann::json::parse(f);
        BossAttackTable t;
        for (const auto& pj : j.at("phases")) {
            PhaseData pd;
            pd.name     = pj.at("name").get<std::string>();
            pd.hpEnter  = pj.at("hpEnter").get<float>();
            pd.speedMul = pj.value("speedMul", 1.f);
            for (const auto& wj : pj.at("windows")) {
                AttackWindow w;
                w.clip     = wj.at("clip").get<std::string>();
                w.windup   = wj.at("windup").get<float>();
                w.active   = wj.at("active").get<float>();
                w.recovery = wj.at("recovery").get<float>();
                if (wj.contains("hitbox")) {
                    const auto& hj = wj.at("hitbox");
                    AttackHitbox hb;
                    auto off = hj.at("offset").get<std::vector<float>>();
                    auto sz  = hj.at("size").get<std::vector<float>>();
                    hb.offset   = {off[0], off[1]};
                    hb.size     = {sz[0],  sz[1]};
                    hb.damage   = hj.at("damage").get<float>();
                    auto kb = hj.at("knockback").get<std::vector<float>>();
                    hb.knockback = {kb[0], kb[1]};
                    w.hitbox = hb;
                }
                if (wj.contains("dashVel")) {
                    auto dv = wj.at("dashVel").get<std::vector<float>>();
                    w.dashVel = glm::vec2{dv[0], dv[1]};
                }
                if (wj.contains("radial")) {
                    const auto& rj = wj.at("radial");
                    w.radial = RadialData{
                        rj.at("count").get<int>(),
                        rj.at("speed").get<float>(),
                        rj.at("damage").get<float>()
                    };
                }
                pd.windows.push_back(std::move(w));
            }
            t.phases.push_back(std::move(pd));
        }
        LOG_INFO("BossAttackTable: {} phases | {} total windows loaded",
                 t.phases.size(),
                 [&]{ int n=0; for(auto& p: t.phases) n+=p.windows.size(); return n; }());
        return t;
    } catch (const std::exception& ex) {
        LOG_WARN("BossAttackTable: parse error '{}': {} — using defaults", path, ex.what());
        return makeDefault();
    }
}

const PhaseData* BossAttackTable::phaseByFraction(float hpFraction) const {
    // Return the last phase whose hpEnter <= hpFraction.
    // Phases are stored in order [stance, enrage, desperate].
    const PhaseData* best = phases.empty() ? nullptr : &phases[0];
    for (const auto& p : phases) {
        if (hpFraction <= p.hpEnter) best = &p;
    }
    return best;
}

} // namespace data
