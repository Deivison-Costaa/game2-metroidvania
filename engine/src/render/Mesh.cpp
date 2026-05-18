#include "engine/render/Mesh.h"
#include "engine/core/Log.h"
#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <format>
#include <stdexcept>

namespace eng::render {

Mesh::Mesh(unsigned int vao, unsigned int vbo, unsigned int ebo, int indexCount)
    : m_vao(vao), m_vbo(vbo), m_ebo(ebo), m_indexCount(indexCount)
{}

Mesh::~Mesh() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo),
      m_ebo(other.m_ebo), m_indexCount(other.m_indexCount)
{
    other.m_vao = other.m_vbo = other.m_ebo = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        if (m_vao) {
            glDeleteVertexArrays(1, &m_vao);
            glDeleteBuffers(1, &m_vbo);
            glDeleteBuffers(1, &m_ebo);
        }
        m_vao        = other.m_vao;
        m_vbo        = other.m_vbo;
        m_ebo        = other.m_ebo;
        m_indexCount = other.m_indexCount;
        other.m_vao  = other.m_vbo = other.m_ebo = 0;
    }
    return *this;
}

Mesh Mesh::fromFile(std::string_view path) {
    Assimp::Importer importer;
    constexpr unsigned int flags =
        aiProcess_Triangulate       |
        aiProcess_GenSmoothNormals  |
        aiProcess_FlipUVs           |
        aiProcess_CalcTangentSpace  |
        aiProcess_JoinIdenticalVertices;

    const aiScene* scene = importer.ReadFile(path.data(), flags);
    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE))
        throw std::runtime_error(std::format(
            "Mesh::fromFile — failed to load '{}': {}", path, importer.GetErrorString()));

    if (scene->mNumMeshes == 0)
        throw std::runtime_error(std::format("Mesh::fromFile — '{}' has no meshes", path));

    const aiMesh* m = scene->mMeshes[0];

    std::vector<MeshVertex>  vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(m->mNumVertices);
    indices.reserve(static_cast<std::size_t>(m->mNumFaces) * 3);

    for (unsigned int i = 0; i < m->mNumVertices; ++i) {
        MeshVertex v;
        v.pos    = {m->mVertices[i].x, m->mVertices[i].y, m->mVertices[i].z};
        v.normal = {m->mNormals[i].x,  m->mNormals[i].y,  m->mNormals[i].z};
        v.uv     = m->mTextureCoords[0]
            ? glm::vec2(m->mTextureCoords[0][i].x, m->mTextureCoords[0][i].y)
            : glm::vec2(0.f);
        vertices.push_back(v);
    }

    for (unsigned int f = 0; f < m->mNumFaces; ++f) {
        const aiFace& face = m->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    GLuint vao{}, vbo{}, ebo{};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)),
        vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(MeshVertex));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(MeshVertex, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(MeshVertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(MeshVertex, uv)));

    glBindVertexArray(0);

    LOG_INFO("Mesh loaded: '{}' [{} verts, {} tris]",
        path, vertices.size(), indices.size() / 3);
    return Mesh(vao, vbo, ebo, static_cast<int>(indices.size()));
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace eng::render
