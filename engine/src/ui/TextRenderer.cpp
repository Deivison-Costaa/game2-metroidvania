#include "engine/ui/TextRenderer.h"

namespace eng::ui {

void drawText(
    eng::render::SpriteBatch& batch,
    Font& font,
    const std::string& text,
    glm::vec2 screenPos,
    glm::vec4 color,
    float scale)
{
    if (!font.valid() || text.empty()) return;

    const eng::render::Texture& atlas = font.atlasTex();
    glm::vec2 cursor = screenPos;

    for (char c : text) {
        if (c == '\n') {
            cursor.x  = screenPos.x;
            cursor.y += font.lineHeight() * scale;
            continue;
        }
        const Glyph* g = font.glyph(c);
        if (!g) continue;

        const float w = g->sizePx.x * scale;
        const float h = g->sizePx.y * scale;

        // Top-left of glyph in screen coords (UI camera: y increases downward)
        const float xpos = cursor.x + g->bearingPx.x * scale;
        const float ypos = cursor.y - g->bearingPx.y * scale;

        // Centre of glyph quad (SpriteBatch::draw takes centre + size)
        const glm::vec2 centre{xpos + w * 0.5f, ypos + h * 0.5f};

        batch.draw(atlas, {centre.x, centre.y, 0.f}, {w, h},
                   g->uvMin, g->uvMax, color, 0.f, false);

        cursor.x += g->advancePx * scale;
    }
}

float textWidth(const Font& font, const std::string& text) {
    float width = 0.f;
    for (char c : text) {
        const Glyph* g = font.glyph(c);
        if (g) width += g->advancePx;
    }
    return width;
}

} // namespace eng::ui
