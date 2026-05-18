#pragma once
#include <string_view>

// Forward-declare SDL types to avoid leaking SDL headers into every TU
struct SDL_Window;
using SDL_GLContext = void*;

namespace eng::core {

class Window {
public:
    Window(std::string_view title, int width, int height);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&)                 = delete;
    Window& operator=(Window&&)      = delete;

    void swap() const;
    void setTitle(std::string_view title);

    int          width()  const { return m_width; }
    int          height() const { return m_height; }
    SDL_Window*  handle() const { return m_window; }

private:
    SDL_Window*  m_window {nullptr};
    SDL_GLContext m_context{nullptr};
    int          m_width;
    int          m_height;
};

} // namespace eng::core
