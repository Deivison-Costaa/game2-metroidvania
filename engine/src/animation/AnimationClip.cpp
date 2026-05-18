#include "engine/animation/AnimationClip.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

namespace eng::animation {

AnimationClip AnimationClip::fromFile(std::string_view path) {
    std::ifstream f(path.data());
    if (!f.is_open())
        throw std::runtime_error(std::string("AnimationClip: cannot open '") + path.data() + "'");

    const auto j = nlohmann::json::parse(f);

    AnimationClip clip;
    clip.name = j.value("name", "");
    clip.loop = j.value("loop", true);

    for (const auto& jf : j.at("frames")) {
        Frame fr;
        // "uv": [uMin, vMin, uMax, vMax]
        const auto& uv = jf.at("uv");
        fr.uvMin = {uv[0].get<float>(), uv[1].get<float>()};
        fr.uvMax = {uv[2].get<float>(), uv[3].get<float>()};
        fr.duration = jf.value("duration", 0.1f);
        clip.frames.push_back(fr);
    }

    if (clip.frames.empty())
        throw std::runtime_error(std::string("AnimationClip: no frames in '") + path.data() + "'");

    return clip;
}

} // namespace eng::animation
