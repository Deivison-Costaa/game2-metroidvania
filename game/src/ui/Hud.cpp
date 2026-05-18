#include "ui/Hud.h"
#include "components/Health.h"
#include "components/Transform.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

// Font search paths (tried in order)
static constexpr const char* kFontCandidates[] = {
    ASSET_ROOT "/assets/fonts/Kenney_Future.ttf",
    "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf",
    "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",
    "/usr/share/fonts/google-carlito-fonts/Carlito-Bold.ttf",
};

// ── Init ──────────────────────────────────────────────────────────────────────

void Hud::init(int screenW, int screenH) {
    m_screenW = screenW;
    m_screenH = screenH;

    // Orthographic camera: origin top-left, y grows downward (screen coords)
    m_uiCam = eng::render::Camera::orthographic(
        0.f, static_cast<float>(screenW),
        static_cast<float>(screenH), 0.f,
        -1.f, 1.f);
    m_uiCam.setPosition({0.f, 0.f, 0.f});
    m_uiCam.setTarget({0.f, 0.f, -1.f});

    // White texture for solid-color bar drawing
    m_whiteTex = eng::render::Texture::fromWhite();

    // Font — try each candidate path in order
    for (const char* fp : kFontCandidates) {
        if (std::filesystem::exists(fp)) {
            m_font = eng::ui::Font::loadFromTTF(fp, 20);
            break;
        }
    }
    if (!m_font.valid())
        LOG_WARN("Hud: no font found — HUD text disabled");
}

void Hud::resize(int w, int h) {
    m_screenW = w;
    m_screenH = h;
    m_uiCam   = eng::render::Camera::orthographic(
        0.f, static_cast<float>(w),
        static_cast<float>(h), 0.f,
        -1.f, 1.f);
    m_uiCam.setPosition({0.f, 0.f, 0.f});
    m_uiCam.setTarget({0.f, 0.f, -1.f});
}

// ── Update ────────────────────────────────────────────────────────────────────

void Hud::update(float dt) {
    m_bossFlashTimer = std::max(0.f, m_bossFlashTimer - dt);

    // Advance popups: rise and fade
    for (auto& p : m_popups) p.lifetime -= dt;
    m_popups.erase(
        std::remove_if(m_popups.begin(), m_popups.end(),
                       [](const FloatingText& p){ return p.lifetime <= 0.f; }),
        m_popups.end());
}

void Hud::pushDamagePopup(glm::vec3 worldPos, float damage, glm::vec4 color) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "-%.0f", static_cast<double>(damage));
    m_popups.push_back({std::string(buf), worldPos, 0.8f, 0.8f, color});
}

void Hud::onBossPhaseChange() {
    m_bossFlashTimer = 0.5f;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void Hud::drawBar(eng::render::SpriteBatch& batch,
                  glm::vec2 topLeft, glm::vec2 size,
                  float fraction, glm::vec4 fillColor, glm::vec4 bgColor)
{
    if (!m_whiteTex) return;
    fraction = std::max(0.f, std::min(1.f, fraction));

    // Background
    const glm::vec2 bgCenter{topLeft.x + size.x * 0.5f, topLeft.y + size.y * 0.5f};
    batch.draw(*m_whiteTex, {bgCenter.x, bgCenter.y, 0.f}, size, bgColor);

    // Fill
    if (fraction > 0.f) {
        const float fillW = size.x * fraction;
        const glm::vec2 fillCenter{topLeft.x + fillW * 0.5f, topLeft.y + size.y * 0.5f};
        batch.draw(*m_whiteTex, {fillCenter.x, fillCenter.y, 0.f}, {fillW, size.y}, fillColor);
    }
}

glm::vec2 Hud::worldToScreen(const eng::render::Camera& worldCam, glm::vec3 worldPos) const {
    const glm::mat4 vp = worldCam.viewProjection();
    glm::vec4 clip = vp * glm::vec4(worldPos, 1.f);
    if (std::abs(clip.w) < 1e-6f) return {-9999.f, -9999.f};
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    return {
        (ndc.x + 1.f) * 0.5f * static_cast<float>(m_screenW),
        (1.f - ndc.y) * 0.5f * static_cast<float>(m_screenH)
    };
}

// ── Render ────────────────────────────────────────────────────────────────────

void Hud::render(eng::render::SpriteBatch& batch,
                 const eng::render::Camera& worldCam,
                 eng::ecs::Registry& reg,
                 eng::ecs::Entity player,
                 eng::ecs::Entity boss)
{
    // Render all HUD elements in UI camera (screen) space.
    // Disable depth test so HUD always appears on top.
    glDisable(GL_DEPTH_TEST);

    batch.begin(m_uiCam);

    // ── Player HP bar (top-left corner) ──────────────────────────────────────
    if (reg.valid(player) && reg.has<Health>(player)) {
        const auto& hp = reg.get<Health>(player);
        const float fraction = hp.max > 0.f ? hp.current / hp.max : 0.f;

        // Colour: green → yellow → red based on HP fraction
        glm::vec4 fillCol;
        if (fraction > 0.5f) {
            fillCol = {0.2f, 0.85f, 0.2f, 0.9f};
        } else if (fraction > 0.25f) {
            fillCol = {0.95f, 0.75f, 0.1f, 0.9f};
        } else {
            fillCol = {0.9f, 0.15f, 0.1f, 0.9f};
        }

        constexpr float kBarX = 20.f, kBarY = 18.f;
        constexpr float kBarW = 200.f, kBarH = 18.f;
        drawBar(batch, {kBarX, kBarY}, {kBarW, kBarH},
                fraction, fillCol, {0.15f, 0.15f, 0.15f, 0.85f});

        // "HP" label
        if (m_font.valid()) {
            char txt[24];
            std::snprintf(txt, sizeof(txt), "HP %.0f/%.0f",
                          static_cast<double>(hp.current), static_cast<double>(hp.max));
            eng::ui::drawText(batch, m_font, txt,
                              {kBarX + 4.f, kBarY + 1.f},
                              {1.f, 1.f, 1.f, 1.f}, 0.85f);
        }
    }

    // ── Boss HP bar (top-centre) ──────────────────────────────────────────────
    if (m_bossVisible && reg.valid(boss) && reg.has<Health>(boss)) {
        const auto& hp = reg.get<Health>(boss);
        const float fraction = hp.max > 0.f ? hp.current / hp.max : 0.f;

        // Flash white on phase transition
        const float flash = m_bossFlashTimer / 0.5f;
        glm::vec4 fillCol;
        if (fraction > 0.66f) {
            fillCol = {0.55f, 0.55f, 0.65f, 0.9f}; // Stance — grey-blue
        } else if (fraction > 0.33f) {
            fillCol = {0.95f, 0.55f, 0.1f,  0.9f}; // Enrage — orange
        } else {
            fillCol = {0.85f, 0.15f, 0.1f,  0.9f}; // Desperate — red
        }
        // Blend towards white on flash
        if (flash > 0.f) {
            fillCol = glm::mix(fillCol, glm::vec4(1.f), flash * 0.7f);
        }

        const float kBarW = static_cast<float>(m_screenW) * 0.5f;
        const float kBarH = 22.f;
        const float kBarX = (static_cast<float>(m_screenW) - kBarW) * 0.5f;
        const float kBarY = 10.f;
        drawBar(batch, {kBarX, kBarY}, {kBarW, kBarH},
                fraction, fillCol, {0.15f, 0.15f, 0.15f, 0.85f});

        if (m_font.valid()) {
            const std::string label = "MINI-BOSS";
            const float tw = eng::ui::textWidth(m_font, label) * 0.9f;
            eng::ui::drawText(batch, m_font, label,
                              {kBarX + (kBarW - tw) * 0.5f, kBarY + 2.f},
                              {1.f, 1.f, 1.f, 1.f}, 0.9f);
        }
    }

    batch.end();

    // ── Damage popups (projected from world to screen) ────────────────────────
    if (!m_popups.empty() && m_font.valid()) {
        batch.begin(m_uiCam);
        for (const auto& p : m_popups) {
            const float alpha = p.lifetime / p.maxLifetime;
            // Rise by 40px over lifetime
            const float riseOffset = (1.f - alpha) * 40.f;
            glm::vec2 screen = worldToScreen(worldCam, p.worldPos);
            screen.y -= riseOffset;

            // Cull off-screen
            if (screen.x < -50.f || screen.x > m_screenW + 50.f) continue;
            if (screen.y < -50.f || screen.y > m_screenH + 50.f) continue;

            glm::vec4 col = p.color;
            col.a = alpha;
            eng::ui::drawText(batch, m_font, p.text, screen, col, 1.1f);
        }
        batch.end();
    }

    glEnable(GL_DEPTH_TEST);
}
