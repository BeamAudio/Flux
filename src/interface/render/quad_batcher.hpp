#ifndef QUAD_BATCHER_HPP
#define QUAD_BATCHER_HPP

#include <vector>
#include <string>
#include "glad.h"

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
     * @brief Draws a quad with a vertical gradient.
     */
    void drawGradientRect(float x, float y, float w, float h, 
                          float r1, float g1, float b1, float a1,
                          float r2, float g2, float b2, float a2);

    /**
     * @brief Draws a rounded rectangle with customizable softness (SDF-based).
     */
    void drawRoundedRect(float x, float y, float w, float h, float radius, float softness, float r, float g, float b, float a);

    /**
     * @brief Draws a rounded rectangle with a vertical gradient.
     */
    void drawRoundedGradientRect(float x, float y, float w, float h, float radius, float softness,
                                 float r1, float g1, float b1, float a1,
                                 float r2, float g2, float b2, float a2);

    /**
     * @brief Draws a beveled rounded rectangle (mode 4).
     */
    void drawBeveledRect(float x, float y, float w, float h, float radius, float softness, float r, float g, float b, float a);

    /**
     * @brief Draws a realistic chassis panel with noise and bevel (mode 5).
     */
    void drawChassisPanel(float x, float y, float w, float h, float radius, float r, float g, float b, float a);

    /**
     * @brief Draws bitmap text using the internal font.
     */
    void drawText(const std::string& text, float x, float y, float size, float r, float g, float b, float a);
    
    /**
     * @brief Draws text using a procedural vector font (lines). Use for scalable, sharp technical text.
     */
    void drawVectorText(const std::string& text, float x, float y, float size, float r, float g, float b, float a);
    
    /**
     * @brief Returns the proportional width of a character for the vector font.
     */
    float getCharWidth(char c, float size) const;

    /**
     * @brief Measures the total width of a string using the vector font proportional logic.
     */
    float getVectorTextWidth(const std::string& text, float size) const;

    /**
     * @brief Saves current OpenGL state to prevent VST corruption.
     */
    void saveState();

    /**
     * @brief Restores previously saved OpenGL state.
     */
    void restoreState();

    /**
     * @brief Draws an additive glow (bloom) quad.
     */
    void drawGlow(float x, float y, float w, float h, float radius, float r, float g, float b, float a);
    
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

    // --- Clipping System ---
    /**
     * @brief Pushes a new clipping rectangle (intersection with current).
     * @param rect The clipping rectangle in world/screen coordinates.
     * @param screenHeight Needed to flip Y for OpenGL.
     */
    void pushClip(float x, float y, float w, float h, float screenHeight);
    
    /**
     * @brief Pops the last clipping rectangle, restoring the previous state.
     */
    void popClip(float screenHeight);

    /**
     * @brief Pushes a coordinate offset for all subsequent drawing operations.
     */
    void pushOffset(float x, float y);
    void popOffset();

    // Accessors for FBO rendering
    float getOffsetX() const { return m_currentOffsetX; }
    float getOffsetY() const { return m_currentOffsetY; }
    void setOffset(float x, float y) { m_currentOffsetX = x; m_currentOffsetY = y; }

    struct ViewTransform {
        float tx, ty, zoom, originX, originY;
    };
    std::vector<ViewTransform> m_transformStack;

    // View Transform Stack (for robust nested views)
    void pushViewTransform();
    void popViewTransform();

    // Deprecated single-level scissor
    void setScissor(float x, float y, float w, float h, float screenHeight);
    void clearScissor();

    void setViewTransform(float tx, float ty, float zoom, float originX = 0.0f, float originY = 0.0f);
    void resetViewTransform(float screenW, float screenH);

    void setShader(class Shader* shader) { m_shader = shader; }

private:
    void createFontTexture();
    size_t m_maxQuads;
    size_t m_quadCount;
    unsigned int m_vao, m_vbo, m_ibo;
    unsigned int m_fontTexture;
    std::vector<Vertex> m_vertices;
    class Shader* m_shader = nullptr;
    float m_viewTx = 0.0f, m_viewTy = 0.0f, m_viewZoom = 1.0f;
    float m_viewOriginX = 0.0f, m_viewOriginY = 0.0f;

    struct ScissorRect {
        float x, y, w, h;
    };
    std::vector<ScissorRect> m_scissorStack;

    float m_currentOffsetX = 0.0f;
    float m_currentOffsetY = 0.0f;
    std::vector<float> m_offsetXStack;
    std::vector<float> m_offsetYStack;

    // Batch State Tracking
    int m_currentMode = 0;
    int m_currentBlendMode = 0; // 0 = Alpha, 1 = Additive
    unsigned int m_currentTextureId = 0;
};

} // namespace Beam

#endif // QUAD_BATCHER_HPP





