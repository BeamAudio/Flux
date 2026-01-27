#include "quad_batcher.hpp"

namespace Beam {

QuadBatcher::QuadBatcher(size_t maxQuads) : m_maxQuads(maxQuads), m_quadCount(0) {
    m_vertices.reserve(maxQuads * 4);

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ibo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, maxQuads * 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    // Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0);
    // TexCoords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(2 * sizeof(float)));
    // Colors
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(4 * sizeof(float)));

    std::vector<unsigned int> indices(maxQuads * 6);
    unsigned int offset = 0;
    for (size_t i = 0; i < maxQuads * 6; i += 6) {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;
        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;
        offset += 4;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

QuadBatcher::~QuadBatcher() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ibo);
}

void QuadBatcher::begin() {
    m_quadCount = 0;
    m_vertices.clear();
}

void QuadBatcher::drawQuad(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (m_quadCount >= m_maxQuads) {
        flush();
        begin();
    }

    m_vertices.push_back({{x, y}, {0, 0}, {r, g, b, a}});
    m_vertices.push_back({{x + w, y}, {1, 0}, {r, g, b, a}});
    m_vertices.push_back({{x + w, y + h}, {1, 1}, {r, g, b, a}});
    m_vertices.push_back({{x, y + h}, {0, 1}, {r, g, b, a}});

    m_quadCount++;
}

void QuadBatcher::end() {
    flush();
}

void QuadBatcher::flush() {
    if (m_quadCount == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)(m_quadCount * 6), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

} // namespace Beam
