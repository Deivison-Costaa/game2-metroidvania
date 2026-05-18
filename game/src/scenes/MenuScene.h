#pragma once
#include "scenes/SceneId.h"
#include "engine/input/InputManager.h"
#include "engine/render/Camera.h"
#include "engine/render/SpriteBatch.h"
#include "engine/render/Texture.h"
#include "engine/ui/Font.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class MenuScene {
public:
    enum class Page { Main, Pause, GameOver, Settings };

    void init(int screenW, int screenH, const std::string& assetRoot, bool hasSave);
    void resize(int w, int h);
    void setPage(Page page);
    void setHasSave(bool v);

    MenuAction update(float dt, const eng::input::InputManager& input);
    void render(bool isOverlay = false);

    float masterVol() const { return m_masterVol; }
    float musicVol()  const { return m_musicVol; }
    float sfxVol()    const { return m_sfxVol; }

    void setVolumes(float master, float music, float sfx) {
        m_masterVol = master; m_musicVol = music; m_sfxVol = sfx;
    }

private:
    struct Button { std::string label; MenuAction action; bool isSlider{false}; };
    struct PageDef { std::string title; std::vector<Button> buttons; };

    void buildPages();
    PageDef& currentPage();
    void drawBar(glm::vec2 topLeft, glm::vec2 size, float frac, glm::vec4 fill, glm::vec4 bg);
    void applyVolumesToAudio();

    std::unique_ptr<eng::render::SpriteBatch> m_batch;
    eng::ui::Font        m_font;
    eng::render::Camera  m_uiCam{eng::render::Camera::orthographic(0.f, 1280.f, 720.f, 0.f, -1.f, 1.f)};
    std::optional<eng::render::Texture> m_whiteTex;

    Page m_page{Page::Main};
    int  m_selected{0};
    std::vector<PageDef> m_pages; // indexed by Page cast to int

    int  m_screenW{1280}, m_screenH{720};
    bool m_hasSave{false};

    float m_masterVol{1.f};
    float m_musicVol {0.7f};
    float m_sfxVol   {1.f};
    float m_repeatTimer{0.f};
    bool  m_prevUp{false}, m_prevDown{false}, m_prevLeft{false}, m_prevRight{false};
};
