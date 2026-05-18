#pragma once
#include <string_view>

namespace eng::render {

enum class TextureFilter { Nearest, Linear };

class Texture {
public:
    static Texture fromFile(std::string_view path,
                            TextureFilter filter = TextureFilter::Nearest);
    static Texture fromWhite();
    // Adopt an existing GL texture ID (Font atlas, etc.). Destructor will delete it.
    static Texture fromRawOwned(unsigned int glId, int w, int h);

    ~Texture();
    Texture(Texture&&) noexcept;
    Texture& operator=(Texture&&) noexcept;
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    void        bind(unsigned int unit = 0) const;
    static void unbind(unsigned int unit = 0);

    unsigned int id()       const { return m_id; }
    int          width()    const { return m_width; }
    int          height()   const { return m_height; }
    int          channels() const { return m_channels; }

private:
    explicit Texture(unsigned int id, int w, int h, int ch);

    unsigned int m_id      {0};
    int          m_width   {0};
    int          m_height  {0};
    int          m_channels{0};
};

} // namespace eng::render
