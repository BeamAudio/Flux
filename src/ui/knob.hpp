#ifndef KNOB_HPP
#define KNOB_HPP

#include "component.hpp"
#include "../graphics/quad_batcher.hpp"
#include <functional>
#include <algorithm>

namespace Beam {

class Knob : public Component {
public:
    Knob(const std::string& label, float minVal, float maxVal, float initialVal)
        : m_label(label), m_min(minVal), m_max(maxVal), m_value(initialVal) {}

    void render(QuadBatcher& batcher) override {
        // Background track
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.15f, 0.15f, 0.15f, 1.0f);
        
        // Value "Fill" (Blue bar)
        float normalized = (m_value - m_min) / (m_max - m_min);
        float fillHeight = m_bounds.h * normalized;
        batcher.drawQuad(m_bounds.x, m_bounds.y + (m_bounds.h - fillHeight), m_bounds.w, fillHeight, 0.25f, 0.5f, 1.0f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button) override {
        m_isDragging = true;
        m_lastY = y;
        return true;
    }

    bool onMouseUp(float x, float y, int button) override {
        m_isDragging = false;
        return true;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isDragging) {
            float deltaY = m_lastY - y;
            float range = m_max - m_min;
            float sensitivity = 0.005f;
            
            m_value += deltaY * range * sensitivity;
            m_value = std::clamp(m_value, m_min, m_max);
            
            if (onValueChanged) {
                onValueChanged(m_value);
            }
            
            m_lastY = y;
            return true;
        }
        return false;
    }

    float getValue() const { return m_value; }
    void setValue(float v) { m_value = std::clamp(v, m_min, m_max); }

    std::function<void(float)> onValueChanged;

private:
    std::string m_label;
    float m_min, m_max, m_value;
    bool m_isDragging = false;
    float m_lastY = 0;
};

} // namespace Beam

#endif // KNOB_HPP
