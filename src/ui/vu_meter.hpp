#ifndef VU_METER_HPP
#define VU_METER_HPP

#include "component.hpp"
#include <cmath>

namespace Beam {

class VUMeter : public Component {
public:
    VUMeter() : m_level(0.0f) {
        setBounds(0, 0, 100, 60);
    }

    void setLevel(float level) { m_level = level; }

    void render(QuadBatcher& batcher) override {
        // Outer case (Rounded plastic look)
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 8.0f, 1.0f, 0.1f, 0.1f, 0.1f, 1.0f);
        
        // Face plate (Cream color)
        batcher.drawRoundedRect(m_bounds.x + 4, m_bounds.y + 4, m_bounds.w - 8, m_bounds.h - 8, 4.0f, 0.5f, 0.9f, 0.88f, 0.8f, 1.0f);

        // Scale markings
        float cx = m_bounds.x + m_bounds.w * 0.5f;
        float cy = m_bounds.y + m_bounds.h + 10; // Pivot point below meter
        
        // Needle
        float angle = -0.8f + m_level * 1.6f; // Map 0..1 to arc
        float needleLen = m_bounds.h * 0.8f;
        float nx = cx + std::sin(angle) * needleLen;
        float ny = cy - std::cos(angle) * needleLen;
        
        batcher.drawLine(cx, cy - 20, nx, ny, 2.0f, 0.8f, 0.1f, 0.1f, 1.0f);
        
        // VU Text
        batcher.drawText("VU", cx - 10, m_bounds.y + m_bounds.h - 20, 12, 0.2f, 0.2f, 0.2f, 0.6f);
    }

private:
    float m_level;
};

} // namespace Beam

#endif // VU_METER_HPP
