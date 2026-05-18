#pragma once
#include <glm/glm.hpp>
#include <string>
#include <string_view>

namespace eng::render {

class Shader {
public:
    static Shader fromFiles(std::string_view vertPath, std::string_view fragPath);
    static Shader fromSource(std::string_view vertSrc,  std::string_view fragSrc);

    ~Shader();
    Shader(Shader&&) noexcept;
    Shader& operator=(Shader&&) noexcept;
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    void        bind()   const;
    static void unbind();
    unsigned int id()    const { return m_id; }

    void set(std::string_view name, float           v) const;
    void set(std::string_view name, int             v) const;
    void set(std::string_view name, const glm::vec2& v) const;
    void set(std::string_view name, const glm::vec3& v) const;
    void set(std::string_view name, const glm::vec4& v) const;
    void set(std::string_view name, const glm::mat4& v) const;

private:
    explicit Shader(unsigned int id);
    int location(std::string_view name) const;

    unsigned int m_id{0};
};

} // namespace eng::render
