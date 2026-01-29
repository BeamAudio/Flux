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

/**
 * @class QuadBatcher
 * @brief High-performance OpenGL renderer for 2D primitives and text.
 */
class QuadBatcher {
public:
    QuadBatcher(size_t maxQuads = 1000);
    ~QuadBatcher();

    /**
     * @brief Prepares the batcher for a new frame.
     */
    void begin();

    /**
     * @brief Draws a solid colored quad.
     */
    void drawQuad(float x, float y, float w, float h, float r, float g, float b, float a);

    /**
     * @brief Draws a rounded rectangle with customizable softness (SDF-based).
     */
    void drawRoundedRect(float x, float y, float w, float h, float radius, float softness, float r, float g, float b, float a);

    /**
     * @brief Draws bitmap text using the internal font.
     */
    void drawText(const std::string& text, float x, float y, float size, float r, float g, float b, float a);
    
    /**
     * @brief Draws a textured quad with specific UV coordinates.
     */
    void drawTexture(unsigned int textureId, float x, float y, float w, float h, 
                     float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f,
                     float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

    /**
     * @brief Draws an anti-aliased line segment using SDF.
     */
    void drawSmoothLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a);

    /**
     * @brief Draws a continuous anti-aliased curve from a set of points.
     */
    void drawCurve(const std::vector<std::pair<float, float>>& points, float thickness, float r, float g, float b, float a);

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


