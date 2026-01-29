#ifndef METER_HPP
#define METER_HPP

#include "component.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace Beam {

/**
 * @class LuminousMeter
 * @brief A professional LED-style level meter with customizable ballistics.
 */
class LuminousMeter : public Component {
public:
    enum class Orientation { Vertical, Horizontal };

    LuminousMeter(Orientation orient = Orientation::Vertical) 
        : m_orientation(orient), m_level(0.0f), m_peak(0.0f) {}

    void setLevel(float linearLevel) {
        if (linearLevel > m_level) m_level = linearLevel; // Instant attack
        if (linearLevel > m_peak) m_peak = linearLevel;
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Ballistics: Slow decay
        m_level -= 1.2f * dt;
        if (m_level < 0.0f) m_level = 0.0f;
        m_peak -= 0.3f * dt;
        if (m_peak < 0.0f) m_peak = 0.0f;

        // Background
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.02f, 0.02f, 0.02f, 1.0f);

        int numSegments = 20;
        float padding = 2.0f;
        
        if (m_orientation == Orientation::Vertical) {
            float segH = (m_bounds.h - (numSegments + 1) * padding) / numSegments;
            for (int i = 0; i < numSegments; ++i) {
                float intensity = (float)i / numSegments;
                float y = m_bounds.y + m_bounds.h - padding - (i + 1) * (segH + padding);
                
                bool active = m_level > intensity;
                float r, g, b;
                
                // Color Gradient: Green -> Yellow -> Red
                if (intensity < 0.7f) { r = 0.2f; g = 0.8f; b = 0.2f; }
                else if (intensity < 0.9f) { r = 0.8f; g = 0.8f; b = 0.2f; }
                else { r = 0.9f; g = 0.2f; b = 0.2f; }

                float alpha = active ? 1.0f : 0.15f;
                batcher.drawQuad(m_bounds.x + padding, y, m_bounds.w - padding * 2, segH, r, g, b, alpha);
                
                // Peak marker
                if (std::abs(m_peak - intensity) < (1.0f / numSegments)) {
                    batcher.drawQuad(m_bounds.x + padding, y, m_bounds.w - padding * 2, 2, 1.0f, 1.0f, 1.0f, 0.8f);
                }
            }
        } else {
            // Horizontal implementation...
            float segW = (m_bounds.w - (numSegments + 1) * padding) / numSegments;
            for (int i = 0; i < numSegments; ++i) {
                float intensity = (float)i / numSegments;
                float x = m_bounds.x + padding + i * (segW + padding);
                bool active = m_level > intensity;
                float r = (intensity < 0.7f) ? 0.2f : (intensity < 0.9f ? 0.8f : 0.9f);
                float g = (intensity < 0.7f) ? 0.8f : (intensity < 0.9f ? 0.8f : 0.2f);
                batcher.drawQuad(x, m_bounds.y + padding, segW, m_bounds.h - padding * 2, r, g, 0.2f, active ? 1.0f : 0.15f);
            }
        }
    }

private:
    Orientation m_orientation;
    float m_level;
    float m_peak;
};

} // namespace Beam

#endif
