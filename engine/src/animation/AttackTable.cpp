#include "engine/animation/AttackTable.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

namespace eng::animation {

AttackTable AttackTable::fromFile(std::string_view path) {
    std::ifstream f{std::string(path)};
    if (!f.is_open())
        throw std::runtime_error(std::string("AttackTable: cannot open ") + std::string(path));

    const auto j = nlohmann::json::parse(f);

    AttackTable table;
    for (auto& [name, entry] : j.items()) {
        AttackData data;
        data.duration = entry.value("duration", 0.25f);

        for (auto& w : entry.at("windows")) {
            AttackWindow win;
            win.start    = w.value("start", 0.f);
            win.end      = w.value("end", data.duration);
            win.damage   = w.value("damage", 1.f);

            if (w.contains("hitbox")) {
                const auto& hb = w.at("hitbox");
                win.offset   = {hb.value("ox", 0.6f), hb.value("oy", 0.f)};
                win.halfSize = {hb.value("hw", 0.4f), hb.value("hh", 0.3f)};
            }

            if (w.contains("knockback")) {
                const auto& kb = w.at("knockback");
                win.knockback = {kb[0].get<float>(), kb[1].get<float>()};
            }

            data.windows.push_back(win);
        }

        table.m_data[name] = std::move(data);
    }
    return table;
}

const AttackData* AttackTable::get(std::string_view name) const {
    auto it = m_data.find(std::string(name));
    return (it != m_data.end()) ? &it->second : nullptr;
}

bool AttackTable::has(std::string_view name) const {
    return m_data.count(std::string(name)) > 0;
}

} // namespace eng::animation
