#ifndef KNOB_HPP
#define KNOB_HPP

#include "component.hpp"
#include <functional>
#include <algorithm>

namespace Beam {

class Knob : public Component {
public:
    Knob(const std::string& label, float minVal, float maxVal, float initialVal)
        : m_label(label), m_min(minVal), m_max(maxVal), m_value(initialVal) {}

    void render() override {
        // In a real implementation, this would use QuadBatcher to draw
        // For now, we'll just track the state.
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
