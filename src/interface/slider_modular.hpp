#ifndef SLIDER_MODULAR_HPP
#define SLIDER_MODULAR_HPP

#include "component.hpp"
#include "../session/parameter.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <string>

namespace Beam {

class ModularSlider : public Component {
public:
    enum class Style { Knob, Vertical, Horizontal };

    ModularSlider(const std::string& label, Style style = Style::Knob) 
        : m_label(label), m_style(style) {
        setBounds(0, 0, 60, 60);
    }

    void bindParameter(std::shared_ptr<Parameter> param) { m_param = param; }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        float val = m_param ? m_param->getNormalizedValue() : 0.5f;
        
        // Label
        batcher.drawText(m_label, m_bounds.x, m_bounds.y - 12, 9, 0.7f, 0.7f, 0.7f, 1.0f);

        if (m_style == Style::Knob) {
            float cx = m_bounds.x + m_bounds.w * 0.5f;
            float cy = m_bounds.y + m_bounds.h * 0.5f;
            float r = (std::min)(m_bounds.w, m_bounds.h) * 0.4f;
            
            // Outer ring
            batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, m_bounds.w*0.5f, 1.0f, 0.1f, 0.1f, 0.12f, 1.0f);
            
            // Value arc (Indicator)
            float angle = -2.356f + val * 4.712f; // -135 to +135 degrees
            float ix = cx + std::sin(angle) * r;
            float iy = cy - std::cos(angle) * r;
            batcher.drawLine(cx, cy, ix, iy, 3.0f, 0.3f, 0.6f, 1.0f, 1.0f);
        } else if (m_style == Style::Vertical) {
            batcher.drawQuad(m_bounds.x + m_bounds.w*0.4f, m_bounds.y, m_bounds.w*0.2f, m_bounds.h, 0.05f, 0.05f, 0.05f, 1.0f);
            float handleY = m_bounds.y + (1.0f - val) * (m_bounds.h - 10);
            batcher.drawRoundedRect(m_bounds.x, handleY, m_bounds.w, 10, 2.0f, 0.5f, 0.7f, 0.2f, 0.2f, 1.0f);
        } else {
            batcher.drawQuad(m_bounds.x, m_bounds.y + m_bounds.h*0.4f, m_bounds.w, m_bounds.h*0.2f, 0.05f, 0.05f, 0.05f, 1.0f);
            float handleX = m_bounds.x + val * (m_bounds.w - 10);
            batcher.drawRoundedRect(handleX, m_bounds.y, 10, m_bounds.h, 2.0f, 0.5f, 0.7f, 0.2f, 0.2f, 1.0f);
        }
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_bounds.contains(x, y)) { m_isDragging = true; return true; }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isDragging && m_param) {
            float delta = 0.0f;
            if (m_style == Style::Vertical) delta = (m_lastY - y) / m_bounds.h;
            else delta = (x - m_lastX) / m_bounds.w;
            
            m_param->setNormalizedValue(std::clamp(m_param->getNormalizedValue() + delta, 0.0f, 1.0f));
            m_lastX = x; m_lastY = y;
            return true;
        }
        m_lastX = x; m_lastY = y;
        return false;
    }

    bool onMouseUp(float x, float y, int button) override { m_isDragging = false; return true; }

private:
    std::string m_label;
    Style m_style;
    std::shared_ptr<Parameter> m_param;
    bool m_isDragging = false;
    float m_lastX = 0, m_lastY = 0;
};

} // namespace Beam
#endif
