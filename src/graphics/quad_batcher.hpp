#ifndef QUAD_BATCHER_HPP
#define QUAD_BATCHER_HPP

#include <vector>
#include <string>
#include "../../third_party/glad.h"

namespace Beam {

struct Vertex {
    float position[2];
    float texCoord[2];
    float color[4];
};

class QuadBatcher {
public:
    QuadBatcher(size_t maxQuads = 1000);
    ~QuadBatcher();

    void begin();
    void drawQuad(float x, float y, float w, float h, float r, float g, float b, float a);
    void drawRoundedRect(float x, float y, float w, float h, float radius, float softness, float r, float g, float b, float a);
    void drawText(const std::string& text, float x, float y, float size, float r, float g, float b, float a);
    void drawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a);
    void drawRect(float x, float y, float w, float h, float thickness, float r, float g, float b, float a);
    void flush();

    void setShader(class Shader* shader) { m_shader = shader; }

private:
    void createFontTexture();
    size_t m_maxQuads;
    size_t m_quadCount;
    unsigned int m_vao, m_vbo, m_ibo;
    unsigned int m_fontTexture;
    std::vector<Vertex> m_vertices;
    class Shader* m_shader = nullptr;
};

} // namespace Beam

#endif // QUAD_BATCHER_HPP
