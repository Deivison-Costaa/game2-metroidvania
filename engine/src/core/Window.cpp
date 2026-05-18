#include "engine/core/Window.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <format>
#include <stdexcept>

namespace eng::core {

Window::Window(std::string_view title, int width, int height)
    : m_width(width), m_height(height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
        throw std::runtime_error(std::format("SDL_Init failed: {}", SDL_GetError()));
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,  8);
    // 4x MSAA — disable if performance is an issue
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    m_window = SDL_CreateWindow(
        title.data(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!m_window) {
        SDL_Quit();
        throw std::runtime_error(std::format("SDL_CreateWindow failed: {}", SDL_GetError()));
    }

    m_context = SDL_GL_CreateContext(m_window);
    if (!m_context) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error(std::format("SDL_GL_CreateContext failed: {}", SDL_GetError()));
    }

    SDL_GL_MakeCurrent(m_window, m_context);
    SDL_GL_SetSwapInterval(1); // vsync on

    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        SDL_GL_DeleteContext(m_context);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        throw std::runtime_error(std::format("glewInit failed: {}",
            reinterpret_cast<const char*>(glewGetErrorString(glewErr))));
    }
    // glewInit may set a benign GL_INVALID_ENUM — clear it
    glGetError();

    LOG_INFO("OpenGL  : {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOG_INFO("GLSL    : {}", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    LOG_INFO("Renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
}

Window::~Window() {
    if (m_context) SDL_GL_DeleteContext(m_context);
    if (m_window)  SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Window::swap() const {
    SDL_GL_SwapWindow(m_window);
}

void Window::setTitle(std::string_view title) {
    SDL_SetWindowTitle(m_window, title.data());
}

} // namespace eng::core
