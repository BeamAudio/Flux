#include "interface/render/quad_batcher.hpp"
#include "interface/render/shader.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Beam {

// Simple 8x8 bitmapped font data for a few ASCII characters
static const unsigned char FONT_DATA[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space
    {0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00}, // !
    {0x6c,0x6c,0x6c,0x00,0x00,0x00,0x00,0x00}, // "
    {0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00}, // #
    {0x18,0x7e,0xc0,0x7c,0x06,0xfc,0x18,0x00}, // $
    {0x00,0xc6,0xcc,0x18,0x30,0x66,0xc6,0x00}, // %
    {0x38,0x6c,0x38,0x76,0xdc,0xcc,0x76,0x00}, // &
    {0x30,0x30,0x60,0x00,0x00,0x00,0x00,0x00}, // '
    {0x18,0x30,0x60,0x60,0x60,0x30,0x18,0x00}, // (
    {0x60,0x30,0x18,0x18,0x18,0x30,0x60,0x00}, // )
    {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00}, // *
    {0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00}, // +
    {0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60}, // ,
    {0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00}, // -
    {0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00}, // .
    {0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0x00}, // /
    {0x7c,0xc6,0xce,0xde,0xf6,0xe6,0x7c,0x00}, // 0
    {0x30,0x70,0x30,0x30,0x30,0x30,0xfc,0x00}, // 1
    {0x7c,0xc6,0x06,0x0c,0x18,0x30,0xfe,0x00}, // 2
    {0x7c,0xc6,0x06,0x3c,0x06,0xc6,0x7c,0x00}, // 3
    {0x1c,0x3c,0x6c,0xcc,0xfe,0x0c,0x1e,0x00}, // 4
    {0xfe,0xc0,0xc0,0xfc,0x06,0xc6,0x7c,0x00}, // 5
    {0x38,0x60,0xc0,0xfc,0xc6,0xc6,0x7c,0x00}, // 6
    {0xfe,0x06,0x0c,0x18,0x30,0x30,0x30,0x00}, // 7
    {0x7c,0xc6,0xc6,0x7c,0xc6,0xc6,0x7c,0x00}, // 8
    {0x7c,0xc6,0xc6,0x7e,0x06,0x0c,0x78,0x00}, // 9
    {0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00}, // :
    {0x00,0x30,0x30,0x00,0x30,0x30,0x60,0x00}, // ;
    {0x18,0x30,0x60,0xc0,0x60,0x30,0x18,0x00}, // <
    {0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00}, // =
    {0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0x00}, // >
    {0x7c,0xc6,0x0c,0x18,0x18,0x00,0x18,0x00}, // ?
    {0x7c,0xc6,0xde,0xde,0xde,0xc0,0x78,0x00}, // @
    {0x38,0x6c,0xc6,0xfe,0xc6,0xc6,0xc6,0x00}, // A
    {0xfc,0x66,0x66,0x7c,0x66,0x66,0xfc,0x00}, // B
    {0x3c,0x66,0xc0,0xc0,0xc0,0x66,0x3c,0x00}, // C
    {0xf8,0x6c,0x66,0x66,0x66,0x6c,0xf8,0x00}, // D
    {0xfe,0x62,0x68,0x78,0x68,0x62,0xfe,0x00}, // E
    {0xfe,0x62,0x68,0x78,0x68,0x60,0xf0,0x00}, // F
    {0x3c,0x66,0xc0,0xc0,0xce,0x66,0x3e,0x00}, // G
    {0xc6,0xc6,0xc6,0xfe,0xc6,0xc6,0xc6,0x00}, // H
    {0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // I
    {0x1e,0x0c,0x0c,0x0c,0x0c,0xcc,0x78,0x00}, // J
    {0xe6,0x66,0x6c,0x78,0x6c,0x66,0xe6,0x00}, // K
    {0xf0,0x60,0x60,0x60,0x62,0x66,0xfe,0x00}, // L
    {0xc6,0xee,0xfe,0xfe,0xd6,0xc6,0xc6,0x00}, // M
    {0xc6,0xe6,0xf6,0xde,0xce,0xc6,0xc6,0x00}, // N
    {0x7c,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x00}, // O
    {0xfc,0x66,0x66,0x7c,0x60,0x60,0xf0,0x00}, // P
    {0x7c,0xc6,0xc6,0xc6,0xd6,0xcc,0x7e,0x00}, // Q
    {0xfc,0x66,0x66,0x7c,0x6c,0x66,0xe6,0x00}, // R
    {0x7c,0xc6,0x60,0x38,0x0c,0xc6,0x7c,0x00}, // S
    {0x7e,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // T
    {0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x00}, // U
    {0xc6,0xc6,0xc6,0xc6,0xc6,0x6c,0x38,0x00}, // V
    {0xc6,0xc6,0xc6,0xd6,0xfe,0xee,0xc6,0x00}, // W
    {0xc6,0xc6,0x6c,0x38,0x6c,0xc6,0xc6,0x00}, // X
    {0xc6,0xc6,0x6c,0x38,0x18,0x18,0x3c,0x00}, // Y
    {0xfe,0x06,0x0c,0x18,0x30,0x60,0xfe,0x00}, // Z
    {0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00}, // [
    {0xc0,0x60,0x30,0x18,0x0c,0x06,0x02,0x00}, // backward slash
    {0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00}, // ]
    {0x10,0x38,0x6c,0xc6,0x00,0x00,0x00,0x00}, // ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff}, // _
    {0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00}, // `
    {0x00,0x00,0x78,0x0c,0x7c,0xcc,0x76,0x00}, // a
    {0xe0,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00}, // b
    {0x00,0x00,0x3c,0x66,0x60,0x66,0x3c,0x00}, // c
    {0x1c,0x0c,0x7c,0xcc,0xcc,0xcc,0x76,0x00}, // d
    {0x00,0x00,0x7c,0xc6,0xfe,0x60,0x3c,0x00}, // e
    {0x1c,0x30,0x7c,0x30,0x30,0x30,0x30,0x00}, // f
    {0x00,0x00,76,0xcc,0xcc,0x7c,0x0c,0xf8}, // g
    {0xe0,0x60,0x6c,0x76,0x66,0x66,0x66,0x00}, // h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3c,0x00}, // i
    {0x06,0x00,0x06,0x06,0x06,0x66,0x3c,0x00}, // j
    {0xe0,0x60,0x66,0x6c,0x78,0x6c,0x66,0x00}, // k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // l
    {0x00,0x00,0xec,0xfe,0xd6,0xd6,0xd6,0x00}, // m
    {0x00,0x00,0xdc,0x66,0x66,0x66,0x66,0x00}, // n
    {0x00,0x00,0x7c,0xc6,0xc6,0xc6,0x7c,0x00}, // o
    {0x00,0x00,0x7c,0x66,0x66,0x7c,0x60,0xf0}, // p
    {0x00,0x00,0x76,0xcc,0xcc,0x7c,0x0c,0x1e}, // q
    {0x00,0x00,0xdc,0x76,0x60,0x60,0xf0,0x00}, // r
    {0x00,0x00,0x7c,0xc0,0x78,0x06,0xfc,0x00}, // s
    {0x30,0x30,0xfc,0x30,0x30,0x30,0x1c,0x00}, // t
    {0x00,0x00,0xcc,0xcc,0xcc,0xcc,0x76,0x00}, // u
    {0x00,0x00,0xcc,0xcc,0xcc,0x78,0x30,0x00}, // v
    {0x00,0x00,0xc6,0xd6,0xd6,0xfe,0x6c,0x00}, // w
    {0x00,0x00,0xc6,0x6c,0x38,0x6c,0xc6,0x00}, // x
    {0x00,0x00,0xc6,0xc6,0xc6,0x7e,0x06,0xfc}, // y
    {0x00,0x00,0xfe,0x0c,0x18,0x30,0xfe,0x00}, // z
    {0x0c,0x18,0x18,0x70,0x18,0x18,0x0c,0x00}, // {
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // |
    {0x30,0x18,0x18,0x0e,0x18,0x18,0x30,0x00}, // }
    {0x00,0x00,0x00,0x76,0xdc,0x00,0x00,0x00}  // ~
};

QuadBatcher::QuadBatcher(size_t maxQuads) : m_maxQuads(maxQuads), m_quadCount(0), m_currentMode(0), m_currentTextureId(0) {
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

    createFontTexture();
}

QuadBatcher::~QuadBatcher() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ibo);
    glDeleteTextures(1, &m_fontTexture);
}

void QuadBatcher::createFontTexture() {
    // Create a 128x128 texture for the font
    unsigned char pixels[128 * 128];
    std::fill(pixels, pixels + 128 * 128, 0);

    for (int i = 0; i < 95; ++i) {
        int tx = (i % 16) * 8;
        int ty = (i / 16) * 8;
        for (int y = 0; y < 8; ++y) {
            unsigned char row = FONT_DATA[i][y];
            for (int x = 0; x < 8; ++x) {
                if (row & (0x80 >> x)) {
                    pixels[(ty + y) * 128 + (tx + x)] = 255;
                }
            }
        }
    }

    glGenTextures(1, &m_fontTexture);
    glBindTexture(GL_TEXTURE_2D, m_fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 128, 128, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void QuadBatcher::begin() {
    m_quadCount = 0;
    m_vertices.clear();
    m_currentMode = -1; 
    m_currentBlendMode = 0;
    m_currentTextureId = 0;
}

void QuadBatcher::drawQuad(float x, float y, float w, float h, float r, float g, float b, float a) {
    if (m_currentMode != 0 || m_currentBlendMode != 0) {
        flush();
        m_currentMode = 0;
        m_currentBlendMode = 0; // Standard Alpha
        m_currentTextureId = 0;
    }
    if (m_quadCount >= m_maxQuads) flush();
    
    float ox = x + m_currentOffsetX;
    float oy = y + m_currentOffsetY;

    m_vertices.push_back({{ox, oy}, {0, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy}, {1, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy + h}, {1, 1}, {r, g, b, a}});
    m_vertices.push_back({{ox, oy + h}, {0, 1}, {r, g, b, a}});
    m_quadCount++;
}

void QuadBatcher::drawGradientRect(float x, float y, float w, float h, 
                                  float r1, float g1, float b1, float a1,
                                  float r2, float g2, float b2, float a2) {
    if (m_currentMode != 0 || m_currentBlendMode != 0) {
        flush();
        m_currentMode = 0;
        m_currentBlendMode = 0;
        m_currentTextureId = 0;
    }
    if (m_quadCount >= m_maxQuads) flush();

    float ox = x + m_currentOffsetX;
    float oy = y + m_currentOffsetY;

    m_vertices.push_back({{ox, oy}, {0, 0}, {r1, g1, b1, a1}});
    m_vertices.push_back({{ox + w, oy}, {1, 0}, {r1, g1, b1, a1}});
    m_vertices.push_back({{ox + w, oy + h}, {1, 1}, {r2, g2, b2, a2}});
    m_vertices.push_back({{ox, oy + h}, {0, 1}, {r2, g2, b2, a2}});
    m_quadCount++;
}

void QuadBatcher::drawRoundedRect(float x, float y, float w, float h, float radius, float softness, float r, float g, float b, float a) {
    drawRoundedGradientRect(x, y, w, h, radius, softness, r, g, b, a, r, g, b, a);
}

void QuadBatcher::drawRoundedGradientRect(float x, float y, float w, float h, float radius, float softness,
                                         float r1, float g1, float b1, float a1,
                                         float r2, float g2, float b2, float a2) {
    if (m_currentMode != 2 || m_currentBlendMode != 0) {
        flush(); 
        m_currentMode = 2;
        m_currentBlendMode = 0;
        m_currentTextureId = 0;
    } else {
        flush(); // Still flush for mode 2 because uniforms change per-call
    }

    if (m_shader) {
        m_shader->use();
        m_shader->setInt("mode", 2);
        m_shader->setFloat("uRadius", radius);
        m_shader->setFloat("uEdgeSoftness", softness);
        m_shader->setFloat("uSizeX", w);
        m_shader->setFloat("uSizeY", h);
    }

    float ox = x + m_currentOffsetX;
    float oy = y + m_currentOffsetY;

    m_vertices.push_back({{ox, oy}, {0, 0}, {r1, g1, b1, a1}});
    m_vertices.push_back({{ox + w, oy}, {1, 0}, {r1, g1, b1, a1}});
    m_vertices.push_back({{ox + w, oy + h}, {1, 1}, {r2, g2, b2, a2}});
    m_vertices.push_back({{ox, oy + h}, {0, 1}, {r2, g2, b2, a2}});
    m_quadCount++;
}

void QuadBatcher::drawBeveledRect(float x, float y, float w, float h, float radius, float softness, float r, float g, float b, float a) {
    flush();
    m_currentMode = 4;
    m_currentBlendMode = 0;
    m_currentTextureId = 0;

    if (m_shader) {
        m_shader->use();
        m_shader->setInt("mode", 4);
        m_shader->setFloat("uRadius", radius);
        m_shader->setFloat("uEdgeSoftness", softness);
        m_shader->setFloat("uSizeX", w);
        m_shader->setFloat("uSizeY", h);
    }

    float ox = x + m_currentOffsetX;
    float oy = y + m_currentOffsetY;

    m_vertices.push_back({{ox, oy}, {0, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy}, {1, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy + h}, {1, 1}, {r, g, b, a}});
    m_vertices.push_back({{ox, oy + h}, {0, 1}, {r, g, b, a}});
    m_quadCount++;

    flush();
    m_currentMode = 0;
}

void QuadBatcher::drawChassisPanel(float x, float y, float w, float h, float radius, float r, float g, float b, float a) {
    flush();
    m_currentMode = 5;
    m_currentBlendMode = 0;
    m_currentTextureId = 0;

    if (m_shader) {
        m_shader->use();
        m_shader->setInt("mode", 5);
        m_shader->setFloat("uRadius", radius);
        m_shader->setFloat("uEdgeSoftness", 1.0f);
        m_shader->setFloat("uSizeX", w);
        m_shader->setFloat("uSizeY", h);
    }

    float ox = x + m_currentOffsetX;
    float oy = y + m_currentOffsetY;

    m_vertices.push_back({{ox, oy}, {0, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy}, {1, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy + h}, {1, 1}, {r, g, b, a}});
    m_vertices.push_back({{ox, oy + h}, {0, 1}, {r, g, b, a}});
    m_quadCount++;

    flush();
    m_currentMode = 0;
}

void QuadBatcher::drawTexture(unsigned int textureId, float x, float y, float w, float h, 
                              float u0, float v0, float u1, float v1,
                              float r, float g, float b, float a) 
{
    if (m_currentMode != 1 || m_currentTextureId != textureId || m_currentBlendMode != 0) {
        flush();
        m_currentMode = 1;
        m_currentBlendMode = 0;
        m_currentTextureId = textureId;
    }

    if (m_quadCount >= m_maxQuads) flush();

    float ox = x + m_currentOffsetX;
    float oy = y + m_currentOffsetY;

    m_vertices.push_back({{ox, oy}, {u0, v0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy}, {u1, v0}, {r, g, b, a}});
    m_vertices.push_back({{ox + w, oy + h}, {u1, v1}, {r, g, b, a}});
    m_vertices.push_back({{ox, oy + h}, {u0, v1}, {r, g, b, a}});

    m_quadCount++;
}

void QuadBatcher::drawSmoothLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a) {
    if (m_currentMode != 3 || m_currentBlendMode != 0) {
        flush();
        m_currentMode = 3;
        m_currentBlendMode = 0;
        m_currentTextureId = 0;
        if (m_shader) m_shader->setFloat("uEdgeSoftness", 0.1f);
    }
    if (m_quadCount >= m_maxQuads) flush();

    float ox1 = x1 + m_currentOffsetX; float oy1 = y1 + m_currentOffsetY;
    float ox2 = x2 + m_currentOffsetX; float oy2 = y2 + m_currentOffsetY;

    float dx = ox2 - ox1;
    float dy = oy2 - oy1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    float nx = -dy / len * thickness * 0.5f;
    float ny = dx / len * thickness * 0.5f;

    m_vertices.push_back({{ox1 + nx, oy1 + ny}, {0, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox2 + nx, oy2 + ny}, {1, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox2 - nx, oy2 - ny}, {1, 1}, {r, g, b, a}});
    m_vertices.push_back({{ox1 - nx, oy1 - ny}, {0, 1}, {r, g, b, a}});
    
    m_quadCount++;
    flush();
    if (m_shader) m_shader->setInt("mode", 0);
}

void QuadBatcher::drawCurve(const std::vector<std::pair<float, float>>& points, float thickness, float r, float g, float b, float a) {
    if (points.size() < 2) return;
    
    for (size_t i = 0; i < points.size() - 1; ++i) {
        drawSmoothLine(points[i].first, points[i].second, 
                       points[i+1].first, points[i+1].second, 
                       thickness, r, g, b, a);
    }
}

void QuadBatcher::drawText(const std::string& text, float x, float y, float size, float r, float g, float b, float a) {
    if (m_currentMode != 1 || m_currentTextureId != m_fontTexture || m_currentBlendMode != 0) {
        flush();
        m_currentMode = 1;
        m_currentBlendMode = 0;
        m_currentTextureId = m_fontTexture;
    }

    float curX = x + m_currentOffsetX;
    float curY = y + m_currentOffsetY;

    for (char c : text) {
        if (c < 32 || c > 126) continue;
        int idx = c - 32;
        float tx = (float)(idx % 16) * 8.0f / 128.0f;
        float ty = (float)(idx / 16) * 8.0f / 128.0f;
        float tw = 8.0f / 128.0f;
        float th = 8.0f / 128.0f;

        m_vertices.push_back({{curX, curY}, {tx, ty}, {r, g, b, a}});
        m_vertices.push_back({{curX + size, curY}, {tx + tw, ty}, {r, g, b, a}});
        m_vertices.push_back({{curX + size, curY + size}, {tx + tw, ty + th}, {r, g, b, a}});
        m_vertices.push_back({{curX, curY + size}, {tx, ty + th}, {r, g, b, a}});

        m_quadCount++;
        curX += getCharWidth(c, size);
        if (m_quadCount >= m_maxQuads) flush(); 
    }
}

void QuadBatcher::drawVectorText(const std::string& text, float x, float y, float size, float r, float g, float b, float a) {
    // Revert to pixel-based rendering as per user request ("Vector things are horrible")
    // Forwarding to drawText uses the internal bitmap font.
    drawText(text, x, y, size, r, g, b, a);
}

float QuadBatcher::getCharWidth(char c, float size) const {
    char up = toupper(c);
    if (up == 'I' || up == '.' || up == ',' || up == ':' || up == ';') return size * 0.55f;
    if (up == ' ') return size * 0.8f;
    if (up == 'L' || up == 'F' || up == 'T' || up == '1') return size * 0.75f;
    if (up == 'M' || up == 'W') return size * 1.0f;
    if (up == 'D' || up == 'G' || up == 'O' || up == 'Q' || up == '@') return size * 0.95f;
    return size * 0.9f; // Generous "Technical" width (monospaced-ish)
}

float QuadBatcher::getVectorTextWidth(const std::string& text, float size) const {
    float total = 0;
    for (char c : text) total += getCharWidth(c, size);
    return total;
}

static GLint s_savedBlendSrcRGB, s_savedBlendDstRGB, s_savedBlendSrcAlpha, s_savedBlendDstAlpha;
static GLboolean s_savedBlend;
static GLint s_savedScissorBox[4];
static GLboolean s_savedScissorTest;

void QuadBatcher::saveState() {
    glGetIntegerv(0x80C9, &s_savedBlendSrcRGB);   // GL_BLEND_SRC_RGB
    glGetIntegerv(0x80C8, &s_savedBlendDstRGB);   // GL_BLEND_DST_RGB
    glGetIntegerv(0x80CB, &s_savedBlendSrcAlpha); // GL_BLEND_SRC_ALPHA
    glGetIntegerv(0x80CA, &s_savedBlendDstAlpha); // GL_BLEND_DST_ALPHA
    s_savedBlend = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_SCISSOR_BOX, s_savedScissorBox);
    s_savedScissorTest = glIsEnabled(GL_SCISSOR_TEST);
}

void QuadBatcher::restoreState() {
    if (s_savedBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    glBlendFunc(s_savedBlendSrcRGB, s_savedBlendDstRGB);
    if (s_savedScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    glScissor(s_savedScissorBox[0], s_savedScissorBox[1], s_savedScissorBox[2], s_savedScissorBox[3]);
}

void QuadBatcher::drawGlow(float x, float y, float w, float h, float radius, float r, float g, float b, float a) {
    if (m_currentBlendMode != 1) {
        flush();
        m_currentBlendMode = 1; // Additive
    }
    
    // Draw using large softness for bloom effect
    drawRoundedRect(x, y, w, h, radius, h * 0.5f, r, g, b, a);
    
    // Note: m_currentBlendMode remains 1 until next draw call resets it.
}

void QuadBatcher::drawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a) {
    if (m_currentMode != 0 || m_currentBlendMode != 0) {
        flush();
        m_currentMode = 0;
        m_currentBlendMode = 0;
        m_currentTextureId = 0;
    }
    if (m_quadCount >= m_maxQuads) flush();

    float ox1 = x1 + m_currentOffsetX; float oy1 = y1 + m_currentOffsetY;
    float ox2 = x2 + m_currentOffsetX; float oy2 = y2 + m_currentOffsetY;

    float dx = ox2 - ox1;
    float dy = oy2 - oy1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    float nx = -dy / len * thickness * 0.5f;
    float ny = dx / len * thickness * 0.5f;

    m_vertices.push_back({{ox1 + nx, oy1 + ny}, {0, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox2 + nx, oy2 + ny}, {1, 0}, {r, g, b, a}});
    m_vertices.push_back({{ox2 - nx, oy2 - ny}, {1, 1}, {r, g, b, a}});
    m_vertices.push_back({{ox1 - nx, oy1 - ny}, {0, 1}, {r, g, b, a}});

    m_quadCount++;
}

void QuadBatcher::drawRect(float x, float y, float w, float h, float thickness, float r, float g, float b, float a) {
    drawQuad(x, y, w, thickness, r, g, b, a); // Top
    drawQuad(x, y + h - thickness, w, thickness, r, g, b, a); // Bottom
    drawQuad(x, y, thickness, h, r, g, b, a); // Left
    drawQuad(x + w - thickness, y, thickness, h, r, g, b, a); // Right
}

void QuadBatcher::pushClip(float x, float y, float w, float h, float screenHeight) {
    flush();

    float wx = x + m_currentOffsetX;
    float wy = y + m_currentOffsetY;
    
    float sx = std::floor(wx * m_viewZoom + m_viewTx + m_viewOriginX);
    float sy = std::floor(wy * m_viewZoom + m_viewTy + m_viewOriginY);
    float sr = std::ceil((wx + w) * m_viewZoom + m_viewTx + m_viewOriginX);
    float sb = std::ceil((wy + h) * m_viewZoom + m_viewTy + m_viewOriginY);

    ScissorRect newRect = { sx, sy, (std::max)(0.0f, sr - sx), (std::max)(0.0f, sb - sy) };

    if (!m_scissorStack.empty()) {
        const auto& current = m_scissorStack.back();
        float x1 = (std::max)(current.x, newRect.x);
        float y1 = (std::max)(current.y, newRect.y);
        float x2 = (std::min)(current.x + current.w, newRect.x + newRect.w);
        float y2 = (std::min)(current.y + current.h, newRect.y + newRect.h);
        
        newRect.x = x1;
        newRect.y = y1;
        newRect.w = (std::max)(0.0f, x2 - x1);
        newRect.h = (std::max)(0.0f, y2 - y1);
    }

    m_scissorStack.push_back(newRect);
    setScissor(newRect.x, newRect.y, newRect.w, newRect.h, screenHeight);
}

void QuadBatcher::popClip(float screenHeight) {
    flush();
    if (!m_scissorStack.empty()) {
        m_scissorStack.pop_back();
    }

    if (m_scissorStack.empty()) {
        glDisable(GL_SCISSOR_TEST);
    } else {
        const auto& r = m_scissorStack.back();
        setScissor(r.x, r.y, r.w, r.h, screenHeight);
    }
}

void QuadBatcher::pushOffset(float x, float y) {
    m_offsetXStack.push_back(m_currentOffsetX);
    m_offsetYStack.push_back(m_currentOffsetY);
    m_currentOffsetX += x;
    m_currentOffsetY += y;
}

void QuadBatcher::popOffset() {
    if (!m_offsetXStack.empty()) {
        m_currentOffsetX = m_offsetXStack.back();
        m_currentOffsetY = m_offsetYStack.back();
        m_offsetXStack.pop_back();
        m_offsetYStack.pop_back();
    }
}

void QuadBatcher::setScissor(float x, float y, float w, float h, float screenHeight) {
    flush();
    glEnable(GL_SCISSOR_TEST);
    int scissorY = (int)(screenHeight - (y + h));
    glScissor((int)x, scissorY, (int)w, (int)h);
}

void QuadBatcher::clearScissor() {
    flush();
    glDisable(GL_SCISSOR_TEST);
}

void QuadBatcher::pushViewTransform() {
    m_transformStack.push_back({m_viewTx, m_viewTy, m_viewZoom, m_viewOriginX, m_viewOriginY});
}

void QuadBatcher::popViewTransform() {
    if (m_transformStack.empty()) return;
    auto& t = m_transformStack.back();
    setViewTransform(t.tx, t.ty, t.zoom, t.originX, t.originY);
    m_transformStack.pop_back();
}

void QuadBatcher::setViewTransform(float tx, float ty, float zoom, float originX, float originY) {
    flush();
    m_viewTx = tx;
    m_viewTy = ty;
    m_viewZoom = zoom;
    m_viewOriginX = originX;
    m_viewOriginY = originY;
}

void QuadBatcher::resetViewTransform(float screenW, float screenH) {
    setViewTransform(0, 0, 1.0f, 0.0f, 0.0f);
}

void QuadBatcher::flush() {
    if (m_quadCount == 0) return;

    if (m_shader) {
        m_shader->use();
        m_shader->setFloat("uPanX", m_viewTx);
        m_shader->setFloat("uPanY", m_viewTy);
        m_shader->setFloat("uOriginX", m_viewOriginX);
        m_shader->setFloat("uOriginY", m_viewOriginY);
        m_shader->setFloat("uZoom", m_viewZoom);

        m_shader->setInt("mode", m_currentMode);
        
        glActiveTexture(0x84C0); // GL_TEXTURE0
        glBindTexture(GL_TEXTURE_2D, m_currentTextureId);
    }

    // Ensure BLEND state is correct for this batch
    glEnable(GL_BLEND);
    if (m_currentBlendMode == 1) {
        glBlendFunc(GL_ONE, GL_ONE); // Additive
    } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // Buffer Orphaning: Tell the driver we don't care about previous content.
    glBufferData(GL_ARRAY_BUFFER, m_maxQuads * 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)(m_quadCount * 6), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    m_quadCount = 0;
    m_vertices.clear();
}

} // namespace Beam