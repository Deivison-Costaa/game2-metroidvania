#include "engine/render/Shader.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <format>

namespace eng::render {

// ── Internal helpers ─────────────────────────────────────────────────────────

static std::string readFile(std::string_view path) {
    std::ifstream f(path.data());
    if (!f.is_open())
        throw std::runtime_error(std::format("Shader file not found: '{}'", path));
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static unsigned int compileStage(GLenum type, std::string_view src) {
    unsigned int id = glCreateShader(type);
    const char*  s  = src.data();
    auto         n  = static_cast<GLint>(src.size());
    glShaderSource(id, 1, &s, &n);
    glCompileShader(id);

    GLint ok = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::size_t>(logLen), '\0');
        glGetShaderInfoLog(id, logLen, nullptr, log.data());
        glDeleteShader(id);
        const char* stage = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        throw std::runtime_error(std::format("[{}] shader compile error:\n{}", stage, log));
    }
    return id;
}

// ── Public API ────────────────────────────────────────────────────────────────

Shader Shader::fromFiles(std::string_view vertPath, std::string_view fragPath) {
    return fromSource(readFile(vertPath), readFile(fragPath));
}

Shader Shader::fromSource(std::string_view vertSrc, std::string_view fragSrc) {
    unsigned int vs   = compileStage(GL_VERTEX_SHADER,   vertSrc);
    unsigned int fs   = compileStage(GL_FRAGMENT_SHADER, fragSrc);
    unsigned int prog = glCreateProgram();

    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(static_cast<std::size_t>(logLen), '\0');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        glDeleteProgram(prog);
        throw std::runtime_error(std::format("Shader link error:\n{}", log));
    }

    return Shader(prog);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

Shader::Shader(unsigned int id) : m_id(id) {}

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

Shader::Shader(Shader&& o) noexcept : m_id(o.m_id) {
    o.m_id = 0;
}

Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (m_id) glDeleteProgram(m_id);
        m_id   = o.m_id;
        o.m_id = 0;
    }
    return *this;
}

// ── Bind ─────────────────────────────────────────────────────────────────────

void Shader::bind()   const { glUseProgram(m_id); }
void Shader::unbind()       { glUseProgram(0); }

// ── Uniforms ─────────────────────────────────────────────────────────────────

int Shader::location(std::string_view name) const {
    return glGetUniformLocation(m_id, name.data());
}

void Shader::set(std::string_view name, float            v) const { glUniform1f(location(name), v); }
void Shader::set(std::string_view name, int              v) const { glUniform1i(location(name), v); }
void Shader::set(std::string_view name, const glm::vec2& v) const { glUniform2fv(location(name), 1, glm::value_ptr(v)); }
void Shader::set(std::string_view name, const glm::vec3& v) const { glUniform3fv(location(name), 1, glm::value_ptr(v)); }
void Shader::set(std::string_view name, const glm::vec4& v) const { glUniform4fv(location(name), 1, glm::value_ptr(v)); }
void Shader::set(std::string_view name, const glm::mat4& v) const { glUniformMatrix4fv(location(name), 1, GL_FALSE, glm::value_ptr(v)); }

} // namespace eng::render
