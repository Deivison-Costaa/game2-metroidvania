#include "scenes/MenuScene.h"
#include "engine/audio/AudioEngine.h"
#include "engine/save/SaveSystem.h"
#include "engine/ui/TextRenderer.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>

// ── init ─────────────────────────────────────────────────────────────────────

void MenuScene::init(int screenW, int screenH, const std::string& assetRoot, bool hasSave) {
    m_screenW  = screenW;
    m_screenH  = screenH;
    m_hasSave  = hasSave;
    m_uiCam    = eng::render::Camera::orthographic(0.f, float(screenW), float(screenH), 0.f, -1.f, 1.f);

    m_batch = std::make_unique<eng::render::SpriteBatch>(
        assetRoot + "/shaders/sprite.vert",
        assetRoot + "/shaders/sprite.frag");

    m_whiteTex = eng::render::Texture::fromWhite();

    const std::string fontPath = assetRoot + "/assets/fonts/Kenney_Future.ttf";
    static constexpr const char* kFallbacks[] = {
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Bold.ttf",
        "/usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/google-carlito-fonts/Carlito-Bold.ttf",
    };
    std::string resolvedFont = fontPath;
    if (!std::ifstream{fontPath}.good()) {
        resolvedFont.clear();
        for (const char* fb : kFallbacks) {
            if (std::ifstream{fb}.good()) { resolvedFont = fb; break; }
        }
    }
    if (!resolvedFont.empty())
        m_font = eng::ui::Font::loadFromTTF(resolvedFont, 22);

    // Sync volumes from AudioEngine
    m_masterVol = eng::audio::AudioEngine::instance().masterVolume();
    m_musicVol  = eng::audio::AudioEngine::instance().musicVolume();
    m_sfxVol    = eng::audio::AudioEngine::instance().sfxVolume();

    buildPages();
}

void MenuScene::buildPages() {
    m_pages.clear();
    m_pages.resize(4); // Main=0, Pause=1, GameOver=2, Settings=3

    // Main menu
    m_pages[0].title   = "GAME2";
    m_pages[0].buttons = {
        {"New Game",  MenuAction::NewGame},
        {"Continue",  MenuAction::Continue},
        {"Settings",  MenuAction::ToSettings},
        {"Quit",      MenuAction::Quit},
    };

    // Pause
    m_pages[1].title   = "PAUSED";
    m_pages[1].buttons = {
        {"Resume",    MenuAction::Resume},
        {"Settings",  MenuAction::ToSettings},
        {"Main Menu", MenuAction::ToMain},
        {"Quit",      MenuAction::Quit},
    };

    // Game Over
    m_pages[2].title   = "GAME OVER";
    m_pages[2].buttons = {
        {"Retry",     MenuAction::Retry},
        {"Main Menu", MenuAction::ToMain},
        {"Quit",      MenuAction::Quit},
    };

    // Settings
    m_pages[3].title   = "SETTINGS";
    m_pages[3].buttons = {
        {"Master Volume", MenuAction::None,            true},
        {"Music Volume",  MenuAction::None,            true},
        {"SFX Volume",    MenuAction::None,            true},
        {"Back",          MenuAction::BackFromSettings, false},
    };
}

void MenuScene::resize(int w, int h) {
    m_screenW = w;
    m_screenH = h;
    m_uiCam   = eng::render::Camera::orthographic(0.f, float(w), float(h), 0.f, -1.f, 1.f);
}

void MenuScene::setPage(Page page) {
    m_page     = page;
    m_selected = 0;
    m_repeatTimer = 0.f;
    // If going to Main, refresh Continue availability (caller sets m_hasSave)
}

void MenuScene::setHasSave(bool v) {
    m_hasSave = v;
    buildPages(); // rebuild so Continue shows correctly
}

MenuScene::PageDef& MenuScene::currentPage() {
    return m_pages[static_cast<int>(m_page)];
}

// ── update ────────────────────────────────────────────────────────────────────

MenuAction MenuScene::update(float dt, const eng::input::InputManager& input) {
    using Act = eng::input::Action;

    auto& pg = currentPage();
    const int  cnt = static_cast<int>(pg.buttons.size());

    // ── Navigation (with initial-press + held repeat) ─────────────────────
    const bool upNow   = input.down(Act::MenuUp);
    const bool downNow = input.down(Act::MenuDown);
    const bool leftNow = input.down(Act::MenuLeft);
    const bool rightNow= input.down(Act::MenuRight);

    const bool upEdge   = upNow   && !m_prevUp;
    const bool downEdge = downNow && !m_prevDown;
    const bool leftEdge = leftNow && !m_prevLeft;
    const bool rightEdge= rightNow&& !m_prevRight;

    bool doUp = upEdge, doDown = downEdge, doLeft = leftEdge, doRight = rightEdge;

    // Held-key repeat (150ms initial delay, then 80ms)
    m_repeatTimer = std::max(0.f, m_repeatTimer - dt);
    const bool anyHeld = upNow || downNow || leftNow || rightNow;
    if (!anyHeld)  m_repeatTimer = 0.f;
    if (anyHeld && !upEdge && !downEdge && !leftEdge && !rightEdge && m_repeatTimer <= 0.f) {
        doUp    = upNow;
        doDown  = downNow;
        doLeft  = leftNow;
        doRight = rightNow;
        m_repeatTimer = 0.08f;
    }
    if (upEdge || downEdge || leftEdge || rightEdge)
        m_repeatTimer = 0.15f;

    m_prevUp    = upNow;
    m_prevDown  = downNow;
    m_prevLeft  = leftNow;
    m_prevRight = rightNow;

    if (doUp)   m_selected = (m_selected - 1 + cnt) % cnt;
    if (doDown) m_selected = (m_selected + 1)       % cnt;

    // ── Settings sliders ─────────────────────────────────────────────────
    if (m_page == Page::Settings) {
        const auto& btn = pg.buttons[m_selected];
        if (btn.isSlider && (doLeft || doRight)) {
            const float delta = doRight ? 0.05f : -0.05f;
            if      (m_selected == 0) m_masterVol = std::clamp(m_masterVol + delta, 0.f, 1.f);
            else if (m_selected == 1) m_musicVol  = std::clamp(m_musicVol  + delta, 0.f, 1.f);
            else if (m_selected == 2) m_sfxVol    = std::clamp(m_sfxVol    + delta, 0.f, 1.f);
            applyVolumesToAudio();
        }
    }

    // ── Confirm ──────────────────────────────────────────────────────────
    if (input.pressed(Act::MenuConfirm)) {
        const auto& btn = pg.buttons[m_selected];
        if (!btn.isSlider) return btn.action;
    }

    // ── Cancel / back ─────────────────────────────────────────────────────
    if (input.pressed(Act::MenuCancel)) {
        if (m_page == Page::Settings) {
            // Persist settings on back
            eng::save::SaveSystem sys;
            if (auto sd = sys.load()) {
                sd->settings.masterVolume = m_masterVol;
                sd->settings.musicVolume  = m_musicVol;
                sd->settings.sfxVolume    = m_sfxVol;
                sys.save(*sd);
            }
            return MenuAction::BackFromSettings;
        }
        if (m_page == Page::Pause)    return MenuAction::Resume;
        if (m_page == Page::Main)     return MenuAction::Quit;
    }

    return MenuAction::None;
}

void MenuScene::applyVolumesToAudio() {
    auto& ae = eng::audio::AudioEngine::instance();
    ae.setMasterVolume(m_masterVol);
    ae.setMusicVolume(m_musicVol);
    ae.setSfxVolume(m_sfxVol);
}

// ── render ────────────────────────────────────────────────────────────────────

void MenuScene::drawBar(glm::vec2 topLeft, glm::vec2 size, float frac,
                        glm::vec4 fill, glm::vec4 bg) {
    if (!m_whiteTex) return;

    auto drawRect = [&](glm::vec2 tl, glm::vec2 sz, glm::vec4 col) {
        const glm::vec2 center = tl + sz * 0.5f;
        m_batch->draw(*m_whiteTex,
                      {center.x, center.y, 0.f},
                      sz, col, 0.f, false);
    };

    drawRect(topLeft, size, bg);
    drawRect(topLeft, {size.x * std::clamp(frac, 0.f, 1.f), size.y}, fill);
}

void MenuScene::render(bool isOverlay) {
    if (!m_batch || !m_whiteTex) return;

    glDisable(GL_DEPTH_TEST);

    const float cx = float(m_screenW) * 0.5f;
    const float cy = float(m_screenH) * 0.5f;

    m_batch->begin(m_uiCam);

    // Background
    if (isOverlay) {
        // Semi-transparent dark overlay over game
        m_batch->draw(*m_whiteTex,
                      {cx, cy, 0.f},
                      {float(m_screenW), float(m_screenH)},
                      {0.f, 0.f, 0.f, 0.6f},
                      0.f, false);
    } else {
        // Solid dark background for standalone menus
        m_batch->draw(*m_whiteTex,
                      {cx, cy, 0.f},
                      {float(m_screenW), float(m_screenH)},
                      {0.04f, 0.03f, 0.06f, 1.f},
                      0.f, false);
    }

    m_batch->end();

    // Text + buttons
    m_batch->begin(m_uiCam);

    auto& pg = currentPage();
    const float titleScale   = 2.2f;
    const float btnScale     = 1.3f;
    const float lineH        = 48.f;
    const float panelTop     = cy - lineH * (float(pg.buttons.size()) + 1.f) * 0.5f;

    // Title
    if (m_font.valid()) {
        const float tw = eng::ui::textWidth(m_font, pg.title) * titleScale;
        eng::ui::drawText(*m_batch, m_font, pg.title,
                          {cx - tw * 0.5f, panelTop - 20.f},
                          {1.f, 1.f, 1.f, 1.f}, titleScale);
    }

    // Buttons
    for (int i = 0; i < static_cast<int>(pg.buttons.size()); ++i) {
        const auto& btn   = pg.buttons[i];
        const bool sel    = (i == m_selected);
        const float btnY  = panelTop + float(i) * lineH + 60.f;
        const float btnW  = 280.f;
        const float btnH  = lineH - 6.f;
        const float btnX  = cx - btnW * 0.5f;

        // Highlight bar for selected
        if (sel) {
            drawBar({btnX - 4.f, btnY - 2.f}, {btnW + 8.f, btnH + 4.f},
                    1.f, {0.25f, 0.15f, 0.5f, 0.75f}, {0.f, 0.f, 0.f, 0.f});
        }

        // Slider
        if (btn.isSlider && m_page == Page::Settings) {
            float val = 1.f;
            if      (i == 0) val = m_masterVol;
            else if (i == 1) val = m_musicVol;
            else if (i == 2) val = m_sfxVol;

            const std::string label = std::format("{} [{:3.0f}%]", btn.label, val * 100.f);
            if (m_font.valid()) {
                const float lw = eng::ui::textWidth(m_font, label) * btnScale;
                eng::ui::drawText(*m_batch, m_font, label,
                                  {cx - lw * 0.5f, btnY},
                                  sel ? glm::vec4{1.f, 0.9f, 0.4f, 1.f} : glm::vec4{0.75f, 0.75f, 0.75f, 1.f},
                                  btnScale);
            }

            // Small slider bar below label
            const float sliderY  = btnY + 22.f;
            const float sliderW  = btnW * 0.6f;
            drawBar({cx - sliderW * 0.5f, sliderY}, {sliderW, 6.f},
                    val,
                    {0.6f, 0.4f, 1.f, 1.f},
                    {0.2f, 0.2f, 0.2f, 1.f});
        } else {
            // Dim "Continue" if no save
            bool dimmed = (btn.action == MenuAction::Continue && !m_hasSave);
            if (m_font.valid()) {
                const float lw = eng::ui::textWidth(m_font, btn.label) * btnScale;
                eng::ui::drawText(*m_batch, m_font, btn.label,
                                  {cx - lw * 0.5f, btnY},
                                  dimmed ? glm::vec4{0.4f, 0.4f, 0.4f, 1.f}
                                  : sel  ? glm::vec4{1.f, 0.9f, 0.4f, 1.f}
                                         : glm::vec4{0.85f, 0.85f, 0.85f, 1.f},
                                  btnScale);
            }
        }
    }

    m_batch->end();
    glEnable(GL_DEPTH_TEST);
}
