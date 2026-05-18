#pragma once
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace eng::save {

struct Settings {
    float masterVolume{1.0f};
    float musicVolume {0.7f};
    float sfxVolume   {1.0f};
};

struct PlayerSave {
    glm::vec2 pos  {0.f, 2.f};
    float     hp   {5.f};
};

struct SaveData {
    int                      version  {1};
    PlayerSave               player;
    std::vector<std::string> defeated; // entity identifiers (e.g. "miniboss", "w0", …)
    float                    playtime {0.f};
    Settings                 settings;
};

// Serialise/deserialise helpers (nlohmann auto-generated for simple structs)
void to_json(nlohmann::json& j, const Settings& s);
void from_json(const nlohmann::json& j, Settings& s);
void to_json(nlohmann::json& j, const PlayerSave& p);
void from_json(const nlohmann::json& j, PlayerSave& p);
void to_json(nlohmann::json& j, const SaveData& d);
void from_json(const nlohmann::json& j, SaveData& d);

class SaveSystem {
public:
    // Returns the canonical save file path:
    //   $XDG_DATA_HOME/game2/save.json  or  ~/.local/share/game2/save.json
    static std::string savePath();

    // Write save atomically (tmp + rename).  Creates directory if needed.
    static void save(const SaveData& data);

    // Load save.  Returns nullopt if file missing/unreadable/wrong version.
    static std::optional<SaveData> load();

    // Delete the save file (used for "new game").
    static void erase();
};

} // namespace eng::save
