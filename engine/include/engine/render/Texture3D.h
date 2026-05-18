#pragma once
#include <string_view>

namespace eng::render {

// RAII wrapper for an OpenGL 3D texture (GL_TEXTURE_3D).
// Primarily used for 3D LUTs loaded from Hald-layout PNG files.
class Texture3D {
public:
    // Load a Hald-layout PNG of size (lutSize²) × lutSize into a 3D texture.
    // lutSize must be a power of two (32 or 64 recommended).
    static Texture3D fromHaldPNG(std::string_view path, int lutSize);

    ~Texture3D();
    Texture3D(Texture3D&&) noexcept;
    Texture3D& operator=(Texture3D&&) noexcept;
    Texture3D(const Texture3D&)            = delete;
    Texture3D& operator=(const Texture3D&) = delete;

    void bind(unsigned int unit = 0) const;

    unsigned int id()   const { return m_id; }
    int          size() const { return m_size; }
    bool         valid() const { return m_id != 0; }

private:
    explicit Texture3D(unsigned int id, int size);

    unsigned int m_id  {0};
    int          m_size{0};
};

} // namespace eng::render
