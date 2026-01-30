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
        
        // Label (Internal)
        batcher.drawText(m_label, m_bounds.x, m_bounds.y, 9, 0.6f, 0.6f, 0.6f, 1.0f);

        float contentY = m_bounds.y + 12.0f;
        float contentH = m_bounds.h - 12.0f;

        if (m_style == Style::Knob) {
            float cx = m_bounds.x + m_bounds.w * 0.5f;
            float cy = contentY + contentH * 0.5f;
            float r = (std::min)(m_bounds.w, contentH) * 0.45f;
            batcher.drawRoundedRect(cx - r, cy - r, r*2, r*2, r, 1.0f, 0.15f, 0.15f, 0.18f, 1.0f);
            float angle = -2.356f + val * 4.712f;
            float ix = cx + std::sin(angle) * r;
            float iy = cy - std::cos(angle) * r;
            batcher.drawLine(cx, cy, ix, iy, 2.5f, 0.4f, 0.7f, 1.0f, 1.0f);
        } else if (m_style == Style::Vertical) {
            batcher.drawQuad(m_bounds.x + m_bounds.w*0.45f, contentY, m_bounds.w*0.1f, contentH, 0.05f, 0.05f, 0.05f, 1.0f);
            float handleY = contentY + (1.0f - val) * (contentH - 8);
            batcher.drawRoundedRect(m_bounds.x, handleY, m_bounds.w, 8, 2.0f, 0.5f, 0.3f, 0.6f, 0.9f, 1.0f);
        } else {
            // Horizontal
            float barH = 4.0f;
            float by = contentY + (contentH - barH) * 0.5f;
            batcher.drawQuad(m_bounds.x, by, m_bounds.w, barH, 0.05f, 0.05f, 0.05f, 1.0f);
            batcher.drawQuad(m_bounds.x, by, m_bounds.w * val, barH, 0.3f, 0.6f, 0.9f, 1.0f);
            float hx = m_bounds.x + val * (m_bounds.w - 6);
            batcher.drawRoundedRect(hx, by - 4, 6, 12, 2.0f, 0.5f, 0.8f, 0.8f, 0.8f, 1.0f);
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
