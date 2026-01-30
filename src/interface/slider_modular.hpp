#ifndef SLIDER_MODULAR_HPP
#define SLIDER_MODULAR_HPP

#include "component.hpp"
#include "../session/parameter.hpp"
#include "../utilities/flux_audio_utils.hpp"
#include <string>
#include <algorithm>

namespace Beam {

class ModularSlider : public Component {
public:
    enum class Style { Knob, Vertical, Horizontal };

    ModularSlider(const std::string& label, Style style = Style::Knob) 
        : m_label(label), m_style(style) {
        setName("ModularSlider");
        setBounds(0, 0, 60, 60);
    }

    void bindParameter(std::shared_ptr<Parameter> param) { m_param = param; }

    void paint(QuadBatcher& g) override;

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        Component::render(batcher, dt, screenW, screenH);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_bounds.contains(x, y)) {
            m_isDragging = true;
            m_lastX = x; m_lastY = y;
            return true;
        }
        return Component::onMouseDown(x, y, button);
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
        return Component::onMouseMove(x, y);
    }

    bool onMouseUp(float x, float y, int button) override { 
        m_isDragging = false; 
        return Component::onMouseUp(x, y, button);
    }

    std::shared_ptr<Parameter> getParameter() const { return m_param; }
    Style getStyle() const { return m_style; }
    const std::string& getLabel() const { return m_label; }

private:
    std::string m_label;
    Style m_style;
    std::shared_ptr<Parameter> m_param;
    bool m_isDragging = false;
    float m_lastX = 0, m_lastY = 0;
};

} // namespace Beam
#endif
