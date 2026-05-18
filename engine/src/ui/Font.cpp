#include "engine/ui/Font.h"
#include "engine/core/Log.h"
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace eng::ui {

const Glyph* Font::glyph(char ch) const noexcept {
    const int idx = static_cast<unsigned char>(ch) - 32;
    if (idx < 0 || idx >= static_cast<int>(m_glyphs.size())) return nullptr;
    return &m_glyphs[idx];
}

Font Font::loadFromTTF(const std::string& path, unsigned int pixelSize) {
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
        throw std::runtime_error("Font: FreeType init failed");

    FT_Face face;
    if (FT_New_Face(ft, path.c_str(), 0, &face)) {
        FT_Done_FreeType(ft);
        throw std::runtime_error("Font: cannot load face: " + path);
    }
    FT_Set_Pixel_Sizes(face, 0, pixelSize);

    constexpr int kFirstChar = 32;
    constexpr int kLastChar  = 126;
    constexpr int kNumGlyphs = kLastChar - kFirstChar + 1;
    constexpr int kAtlasSize = 512;

    // Collect bitmap data per glyph and compute atlas positions
    struct BitmapEntry {
        int x{0}, y{0}, w{0}, h{0};
        int bearingX{0}, bearingY{0};
        int advance{0};
        std::vector<uint8_t> pixels;
    };
    std::vector<BitmapEntry> entries(kNumGlyphs);

    int penX = 0, penY = 0, rowH = 0;
    for (int i = 0; i < kNumGlyphs; ++i) {
        if (FT_Load_Char(face, static_cast<FT_ULong>(kFirstChar + i), FT_LOAD_RENDER))
            continue;
        FT_GlyphSlot g = face->glyph;
        const int    bw = static_cast<int>(g->bitmap.width);
        const int    bh = static_cast<int>(g->bitmap.rows);

        if (penX + bw + 1 >= kAtlasSize) {
            penX  = 0;
            penY += rowH + 1;
            rowH  = 0;
        }
        rowH = std::max(rowH, bh);

        auto& e  = entries[i];
        e.x      = penX;
        e.y      = penY;
        e.w      = bw;
        e.h      = bh;
        e.bearingX = g->bitmap_left;
        e.bearingY = g->bitmap_top;
        e.advance  = static_cast<int>(g->advance.x) >> 6;
        e.pixels.assign(g->bitmap.buffer, g->bitmap.buffer + bw * bh);

        penX += bw + 1;
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Bake RGBA atlas — white text, alpha from glyph bitmap
    std::vector<uint8_t> atlas(kAtlasSize * kAtlasSize * 4, 0);
    const float as = static_cast<float>(kAtlasSize);

    Font font;
    font.m_atlasSize  = kAtlasSize;
    font.m_lineHeight = static_cast<float>(pixelSize);
    font.m_glyphs.resize(kNumGlyphs);

    for (int i = 0; i < kNumGlyphs; ++i) {
        const auto& e = entries[i];
        for (int row = 0; row < e.h; ++row) {
            for (int col = 0; col < e.w; ++col) {
                int dst  = ((e.y + row) * kAtlasSize + (e.x + col)) * 4;
                uint8_t a = e.pixels[row * e.w + col];
                atlas[dst + 0] = 255;
                atlas[dst + 1] = 255;
                atlas[dst + 2] = 255;
                atlas[dst + 3] = a;
            }
        }
        Glyph& gl    = font.m_glyphs[i];
        gl.uvMin     = {e.x / as, e.y / as};
        gl.uvMax     = {(e.x + e.w) / as, (e.y + e.h) / as};
        gl.sizePx    = {static_cast<float>(e.w), static_cast<float>(e.h)};
        gl.bearingPx = {static_cast<float>(e.bearingX), static_cast<float>(e.bearingY)};
        gl.advancePx = static_cast<float>(e.advance);
    }

    // Upload to GL
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kAtlasSize, kAtlasSize,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Hand ownership to Texture (will delete on Font destruction)
    font.m_atlas = eng::render::Texture::fromRawOwned(texId, kAtlasSize, kAtlasSize);

    LOG_INFO("Font loaded: '{}' @ {}px | atlas {}x{}", path, pixelSize, kAtlasSize, kAtlasSize);
    return font;
}

} // namespace eng::ui
