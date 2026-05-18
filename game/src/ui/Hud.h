#pragma once
#include "engine/ecs/Registry.h"
#include "engine/ecs/Entity.h"
#include "engine/render/Camera.h"
#include "engine/render/SpriteBatch.h"
#include "engine/render/Texture.h"
#include "engine/ui/Font.h"
#include "engine/ui/TextRenderer.h"
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

class Hud {
public:
    void init(int screenW, int screenH);
    void update(float dt);

    // Render all HUD elements (must be called after postFx composite, in default FB).
    // worldCam is the gameplay camera used to project damage-popup world positions.
    void render(eng::render::SpriteBatch& batch,
                const eng::render::Camera& worldCam,
                eng::ecs::Registry& reg,
                eng::ecs::Entity player,
                eng::ecs::Entity boss);

    // Spawn a floating damage number at a world-space position.
    void pushDamagePopup(glm::vec3 worldPos, float damage,
                         glm::vec4 color = {1.f, 1.f, 0.3f, 1.f});

    // Called when the boss transitions to a new phase (triggers HP bar flash).
    void onBossPhaseChange();

    void setBossVisible(bool v) { m_bossVisible = v; }
    bool bossVisible() const noexcept { return m_bossVisible; }

    void resize(int w, int h);

private:
    // ── Helpers ──────────────────────────────────────────────────────────────
    void drawBar(eng::render::SpriteBatch& batch,
                 glm::vec2 topLeft, glm::vec2 size,
                 float fraction, glm::vec4 fillColor, glm::vec4 bgColor);

    // Project a world-space point to screen coords under the current UI camera.
    // Returns the 2D screen position (pixels, origin top-left).
    glm::vec2 worldToScreen(const eng::render::Camera& worldCam,
                            glm::vec3 worldPos) const;

    // ── Data ─────────────────────────────────────────────────────────────────
    struct FloatingText {
        std::string text;
        glm::vec3   worldPos;
        float       lifetime;
        float       maxLifetime;
        glm::vec4   color;
    };

    eng::ui::Font               m_font;
    eng::render::Camera         m_uiCam{
        eng::render::Camera::orthographic(0.f, 1280.f, 720.f, 0.f, -1.f, 1.f)};
    std::optional<eng::render::Texture> m_whiteTex;

    std::vector<FloatingText>   m_popups;

    float m_bossFlashTimer{0.f};
    bool  m_bossVisible   {false};

    int   m_screenW{1280};
    int   m_screenH{720};
};
