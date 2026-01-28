#ifndef KNOB_HPP
#define KNOB_HPP

#include "component.hpp"
#include "../render/quad_batcher.hpp"
#include "../render/texture.hpp"
#include "../session/parameter.hpp"
#include <functional>
#include <algorithm>
#include <memory>

namespace Beam {

class Knob : public Component {
public:
    Knob(const std::string& label, float minVal, float maxVal, float initialVal)
        : m_label(label), m_min(minVal), m_max(maxVal), m_value(initialVal) {}

    void bindParameter(std::shared_ptr<Parameter> param) {
        m_parameter = param;
        if (m_parameter) {
            m_min = m_parameter->getMin();
            m_max = m_parameter->getMax();
            m_value = m_parameter->getValue();
            m_label = m_parameter->getName();
        }
    }

    void setTexture(std::shared_ptr<Texture> texture, int numFrames) {
        m_texture = texture;
        m_numFrames = numFrames;
    }

    void render(QuadBatcher& batcher) override {
        if (m_parameter) {
            m_value = m_parameter->getValue();
        }

        float normalized = (m_value - m_min) / (m_max - m_min);

        if (m_texture && m_numFrames > 0) {
            // Filmstrip rendering
            int frame = (int)(normalized * (m_numFrames - 1));
            frame = std::clamp(frame, 0, m_numFrames - 1);

            // Assume vertical filmstrip
            float frameHeight = (float)m_texture->getHeight() / m_numFrames;
            float u0 = 0.0f;
            float u1 = 1.0f;
            float v0 = (float)frame / m_numFrames;
            float v1 = (float)(frame + 1) / m_numFrames;

            // Maintain aspect ratio or fill bounds? 
            // For now, center in bounds
            batcher.drawTexture(m_texture->getID(), m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, u0, v0, u1, v1);

        } else {
            // Fallback Vector Rendering
            float cx = m_bounds.x + m_bounds.w * 0.5f;
            float cy = m_bounds.y + m_bounds.h * 0.5f;
            float radius = (std::min)(m_bounds.w, m_bounds.h) * 0.4f;

            // Label
            batcher.drawText(m_label, m_bounds.x, m_bounds.y - 12, 10, 0.7f, 0.7f, 0.7f, 1.0f);

            // Knob Outer Circle (Body)
            batcher.drawRoundedRect(m_bounds.x + 5, m_bounds.y + 5, m_bounds.w - 10, m_bounds.h - 10, radius, 1.5f, 0.1f, 0.1f, 0.11f, 1.0f);
            
            // Value Indicator Line
            float angle = -2.356f + normalized * 4.712f; // -135 to +135 degrees
            
            float lx = cx + std::sin(angle) * radius;
            float ly = cy - std::cos(angle) * radius;
            
            batcher.drawLine(cx, cy, lx, ly, 3.0f, 1.0f, 0.5f, 0.0f, 1.0f); // Orange needle

            // Center Cap
            batcher.drawRoundedRect(cx - 4, cy - 4, 8, 8, 4.0f, 0.5f, 0.2f, 0.2f, 0.22f, 1.0f);
        }

        // Value text
        char valStr[16];
        snprintf(valStr, 16, "%.2f", m_value);
        batcher.drawText(valStr, m_bounds.x + 5, m_bounds.y + m_bounds.h + 2, 9, 0.5f, 0.5f, 0.5f, 1.0f);
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
            
            if (m_parameter) {
                m_parameter->setValue(m_value);
            }

            if (onValueChanged) {
                onValueChanged(m_value);
            }
            
            m_lastY = y;
            return true;
        }
        return false;
    }

    float getValue() const { return m_value; }
    void setValue(float v) { 
        m_value = std::clamp(v, m_min, m_max); 
        if (m_parameter) m_parameter->setValue(m_value);
    }

    std::function<void(float)> onValueChanged;

private:
    std::string m_label;
    float m_min, m_max, m_value;
    bool m_isDragging = false;
    float m_lastY = 0;
    std::shared_ptr<Parameter> m_parameter;
    
    std::shared_ptr<Texture> m_texture;
    int m_numFrames = 0;
};

} // namespace Beam

#endif // KNOB_HPP

