#include "engine/save/SaveSystem.h"
#include "engine/core/Log.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace eng::save {

// ── JSON serialisation ────────────────────────────────────────────────────────

void to_json(nlohmann::json& j, const Settings& s) {
    j = {{"masterVolume", s.masterVolume},
         {"musicVolume",  s.musicVolume},
         {"sfxVolume",    s.sfxVolume}};
}
void from_json(const nlohmann::json& j, Settings& s) {
    j.at("masterVolume").get_to(s.masterVolume);
    j.at("musicVolume") .get_to(s.musicVolume);
    j.at("sfxVolume")   .get_to(s.sfxVolume);
}

void to_json(nlohmann::json& j, const PlayerSave& p) {
    j = {{"pos", {p.pos.x, p.pos.y}}, {"hp", p.hp}};
}
void from_json(const nlohmann::json& j, PlayerSave& p) {
    auto arr = j.at("pos").get<std::vector<float>>();
    p.pos = {arr.at(0), arr.at(1)};
    j.at("hp").get_to(p.hp);
}

void to_json(nlohmann::json& j, const SaveData& d) {
    j = {{"version",  d.version},
         {"player",   d.player},
         {"defeated", d.defeated},
         {"playtime", d.playtime},
         {"settings", d.settings}};
}
void from_json(const nlohmann::json& j, SaveData& d) {
    j.at("version") .get_to(d.version);
    j.at("player")  .get_to(d.player);
    j.at("defeated").get_to(d.defeated);
    j.at("playtime").get_to(d.playtime);
    j.at("settings").get_to(d.settings);
}

// ── Path resolution ───────────────────────────────────────────────────────────

std::string SaveSystem::savePath() {
    const char* xdg = std::getenv("XDG_DATA_HOME");
    fs::path base;
    if (xdg && *xdg) {
        base = fs::path(xdg) / "game2";
    } else {
        const char* home = std::getenv("HOME");
        if (!home || !*home)
            throw std::runtime_error("SaveSystem: cannot determine HOME directory");
        base = fs::path(home) / ".local" / "share" / "game2";
    }
    return (base / "save.json").string();
}

// ── IO ────────────────────────────────────────────────────────────────────────

void SaveSystem::save(const SaveData& data) {
    const std::string path = savePath();
    const std::string tmp  = path + ".tmp";

    // Ensure parent directory exists
    fs::create_directories(fs::path(path).parent_path());

    {
        std::ofstream f(tmp);
        if (!f) throw std::runtime_error("SaveSystem: cannot write " + tmp);
        f << nlohmann::json(data).dump(2);
    }

    // Atomic rename
    fs::rename(tmp, path);
    LOG_INFO("SaveSystem: saved → {}", path);
}

std::optional<SaveData> SaveSystem::load() {
    const std::string path = savePath();
    std::ifstream f(path);
    if (!f) return std::nullopt;

    try {
        auto j    = nlohmann::json::parse(f);
        SaveData d = j.get<SaveData>();
        if (d.version != 1) {
            LOG_WARN("SaveSystem: version mismatch (got {}) — ignoring save", d.version);
            return std::nullopt;
        }
        LOG_INFO("SaveSystem: loaded '{}' (v{}, playtime {:.1f}s)", path, d.version, d.playtime);
        return d;
    } catch (const std::exception& ex) {
        LOG_WARN("SaveSystem: parse error '{}': {}", path, ex.what());
        return std::nullopt;
    }
}

void SaveSystem::erase() {
    try { fs::remove(savePath()); } catch (...) {}
}

} // namespace eng::save
