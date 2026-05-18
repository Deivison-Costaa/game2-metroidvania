#pragma once
#include <glm/glm.hpp>
#include <string_view>
#include <vector>

namespace eng::render {

struct MeshVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Mesh {
public:
    static Mesh fromFile(std::string_view path);

    ~Mesh();
    Mesh(Mesh&&) noexcept;
    Mesh& operator=(Mesh&&) noexcept;
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;
    int  indexCount() const { return m_indexCount; }

private:
    explicit Mesh(unsigned int vao, unsigned int vbo, unsigned int ebo, int indexCount);

    unsigned int m_vao{0};
    unsigned int m_vbo{0};
    unsigned int m_ebo{0};
    int          m_indexCount{0};
};

} // namespace eng::render
