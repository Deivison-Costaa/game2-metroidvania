#include "engine/render/Texture3D.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <format>
#include <stdexcept>
#include <vector>

// stb_image included WITHOUT implementation here (defined in Texture.cpp)
#include "stb_image.h"

namespace eng::render {

Texture3D::Texture3D(unsigned int id, int size)
    : m_id(id), m_size(size)
{}

Texture3D::~Texture3D() {
    if (m_id) glDeleteTextures(1, &m_id);
}

Texture3D::Texture3D(Texture3D&& other) noexcept
    : m_id(other.m_id), m_size(other.m_size)
{
    other.m_id = 0;
}

Texture3D& Texture3D::operator=(Texture3D&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id       = other.m_id;
        m_size     = other.m_size;
        other.m_id = 0;
    }
    return *this;
}

void Texture3D::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_3D, m_id);
}

// Hald layout: a PNG of (N² × N) pixels.
//   Pixel at (x, y) with x = b*N + r, y = g  maps to LUT entry [r][g][b].
// We reorganise that into a linear 3D volume [N][N][N] * 3 bytes (RGB8).
Texture3D Texture3D::fromHaldPNG(std::string_view path, int N) {
    stbi_set_flip_vertically_on_load(0);
    int w{}, h{}, ch{};
    unsigned char* raw = stbi_load(path.data(), &w, &h, &ch, 3);
    if (!raw)
        throw std::runtime_error(std::format(
            "Texture3D::fromHaldPNG — failed to load '{}': {}",
            path, stbi_failure_reason()));

    // Expected layout: width = N², height = N
    if (w != N * N || h != N) {
        stbi_image_free(raw);
        throw std::runtime_error(std::format(
            "Texture3D::fromHaldPNG — '{}': expected {}×{} for N={}, got {}×{}",
            path, N * N, N, N, w, h));
    }

    // Rearrange into [b][g][r] volume (GL expects R changes fastest)
    std::vector<unsigned char> vol(N * N * N * 3);
    for (int g = 0; g < N; ++g) {
        for (int b = 0; b < N; ++b) {
            for (int r = 0; r < N; ++r) {
                const int srcX = b * N + r;
                const int srcY = g;
                const int srcOff = (srcY * w + srcX) * 3;
                // Volume index: slice=b, row=g, col=r  → linear (b*N*N + g*N + r)*3
                const int dstOff = (b * N * N + g * N + r) * 3;
                vol[dstOff + 0] = raw[srcOff + 0];
                vol[dstOff + 1] = raw[srcOff + 1];
                vol[dstOff + 2] = raw[srcOff + 2];
            }
        }
    }
    stbi_image_free(raw);

    GLuint id{};
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_3D, id);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, N, N, N, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, vol.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_3D, 0);

    LOG_INFO("Texture3D: loaded LUT {}³ from '{}'", N, path);
    return Texture3D{id, N};
}

} // namespace eng::render
