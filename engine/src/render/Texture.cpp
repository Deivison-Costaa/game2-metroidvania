#include "engine/render/Texture.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <format>
#include <stdexcept>

// stb_image — implementation defined exactly once here
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#pragma GCC diagnostic pop

namespace eng::render {

Texture::Texture(unsigned int id, int w, int h, int ch)
    : m_id(id), m_width(w), m_height(h), m_channels(ch)
{}

Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_width(other.m_width),
      m_height(other.m_height), m_channels(other.m_channels)
{
    other.m_id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteTextures(1, &m_id);
        m_id       = other.m_id;
        m_width    = other.m_width;
        m_height   = other.m_height;
        m_channels = other.m_channels;
        other.m_id = 0;
    }
    return *this;
}

Texture Texture::fromFile(std::string_view path, TextureFilter filter) {
    stbi_set_flip_vertically_on_load(1);
    int w{}, h{}, ch{};
    unsigned char* data = stbi_load(path.data(), &w, &h, &ch, 4);
    if (!data)
        throw std::runtime_error(std::format(
            "Texture::fromFile — failed to load '{}': {}", path, stbi_failure_reason()));

    GLuint id{};
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    const GLint glFilter = (filter == TextureFilter::Nearest) ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    LOG_INFO("Texture loaded: '{}' [{}x{} ch={}]", path, w, h, ch);
    return Texture(id, w, h, ch);
}

Texture Texture::fromWhite() {
    const unsigned char px[4] = {255, 255, 255, 255};
    GLuint id{};
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture(id, 1, 1, 4);
}

void Texture::bind(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind(unsigned int unit) {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace eng::render
