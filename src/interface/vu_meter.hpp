#ifndef VU_METER_HPP
#define VU_METER_HPP

#include "component.hpp"
#include <cmath>
#include <algorithm>

namespace Beam {

class VUMeter : public Component {
public:
    VUMeter() : m_level(0.0f), m_targetLevel(0.0f) {
        setBounds(0, 0, 100, 60);
    }

    void setLevel(float level) { 
        m_targetLevel = level;
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Analog Ballistics (Physics)
        float attack = 15.0f;
        float release = 3.5f;
        float coeff = (m_targetLevel > m_level) ? attack : release;
        m_level += (m_targetLevel - m_level) * coeff * dt;
        m_level = (std::clamp)(m_level, 0.0f, 1.2f);

        // Outer case
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 8.0f, 1.0f, 0.1f, 0.1f, 0.1f, 1.0f);
        // Face plate
        batcher.drawRoundedRect(m_bounds.x + 4, m_bounds.y + 4, m_bounds.w - 8, m_bounds.h - 8, 4.0f, 0.5f, 0.9f, 0.88f, 0.8f, 1.0f);

        float cx = m_bounds.x + m_bounds.w * 0.5f;
        float cy = m_bounds.y + m_bounds.h + 20; 
        float pivotY = cy - 30;

        // Ticks
        float dbValues[] = {-20.0f, -10.0f, -5.0f, 0.0f, 3.0f};
        for (float db : dbValues) {
            float t = 0.0f;
            if (db == -20) t = 0.1f;
            else if (db == -10) t = 0.35f;
            else if (db == -5) t = 0.55f;
            else if (db == 0) t = 0.75f;
            else if (db == 3) t = 0.95f;
            
            float a = -0.8f + t * 1.6f;
            float r1 = m_bounds.h * 0.75f;
            float r2 = m_bounds.h * 0.85f;
            float sx = cx + std::sin(a) * r1;
            float sy = cy - std::cos(a) * r1;
            float ex = cx + std::sin(a) * r2;
            float ey = cy - std::cos(a) * r2;
            
            float col = (db > 0) ? 0.8f : 0.2f;
            batcher.drawLine(sx, sy, ex, ey, 1.5f, col, (db > 0) ? 0.1f : 0.2f, 0.2f, 1.0f);
            
            std::string lbl = std::to_string((int)std::abs(db));
            if (db > 0) lbl = "+" + lbl;
            float tx = cx + std::sin(a) * (r2 + 8);
            float ty = cy - std::cos(a) * (r2 + 8);
            batcher.drawText(lbl, tx - 4, ty - 4, 8, col, (db > 0) ? 0.1f : 0.2f, 0.2f, 1.0f);
        }

        // Needle
        float angle = -0.8f + m_level * 1.6f; 
        float needleLen = m_bounds.h * 0.85f;
        float nx = cx + std::sin(angle) * needleLen;
        float ny = pivotY - std::cos(angle) * needleLen;
        
        batcher.drawLine(cx, pivotY, nx, ny, 2.0f, 0.8f, 0.1f, 0.1f, 1.0f);
        batcher.drawText("VU", cx - 10, m_bounds.y + m_bounds.h - 20, 12, 0.2f, 0.2f, 0.2f, 0.6f);
    }

private:
    float m_level;
    float m_targetLevel;
};

} // namespace Beam

#endif