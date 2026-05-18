#pragma once
#include "engine/render/Texture.h"
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace eng::ui {

struct Glyph {
    glm::vec2 uvMin;     // normalised [0..1] in the atlas texture
    glm::vec2 uvMax;
    glm::vec2 sizePx;    // glyph bitmap size in pixels
    glm::vec2 bearingPx; // offset from cursor baseline to top-left of bitmap
    float     advancePx; // horizontal advance to next glyph
};

class Font {
public:
    Font() = default;

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;
    Font(Font&&) noexcept = default;
    Font& operator=(Font&&) noexcept = default;

    // Load a TTF/OTF file at the given pixel size and bake an atlas texture.
    // Covers ASCII 32..126.
    static Font loadFromTTF(const std::string& path, unsigned int pixelSize = 24);

    bool valid() const noexcept { return m_atlas.has_value(); }
    const eng::render::Texture& atlasTex() const { return *m_atlas; }
    int   atlasSize()   const noexcept { return m_atlasSize; }
    float lineHeight()  const noexcept { return m_lineHeight; }

    // Returns nullptr if ch is out of the covered range.
    const Glyph* glyph(char ch) const noexcept;

private:
    std::optional<eng::render::Texture> m_atlas;
    int                                 m_atlasSize  {512};
    float                               m_lineHeight {24.f};
    std::vector<Glyph>                  m_glyphs;    // index = codepoint - 32
};

} // namespace eng::ui
