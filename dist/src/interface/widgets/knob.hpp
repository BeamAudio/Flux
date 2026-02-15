#ifndef KNOB_HPP
#define KNOB_HPP

#include "interface/core/component.hpp"
#include "interface/core/text_element.hpp"
#include "interface/render/quad_batcher.hpp"
#include "interface/render/texture.hpp"
#include "engine/session/parameter.hpp"
#include "engine/session/undo_manager.hpp"
#include "engine/session/commands.hpp"
#include "interface/core/theme.hpp"
#include "engine/midi/midi_event.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <functional>
#include <algorithm>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#endif

namespace Beam {

/**
 * @class Knob
 * @brief A rotary knob component with a decoupled TextElement for its label.
 */
class Knob : public Component {
public:
    Knob(const std::string& label = "Knob", float minVal = 0.0f, float maxVal = 1.0f, float initialVal = 0.0f)
        : m_min(minVal), m_max(maxVal), m_value(initialVal) {
        setName("Knob");
        
        m_labelElement = std::make_shared<TextElement>(label);
        m_labelElement->setFontSize(11.0f);
        m_labelElement->setJustification(TextElement::Justification::Center);
        m_labelElement->setWrapWidth(60.0f);
        addChildComponent(m_labelElement);
    }

    void bindParameter(std::shared_ptr<Parameter> param) {
        m_parameter = param;
        if (m_parameter) {
            m_min = m_parameter->getMin();
            m_max = m_parameter->getMax();
            m_value = m_parameter->getValue();
            m_labelElement->setText(m_parameter->getName());
        }
    }

    void setParameter(std::shared_ptr<Parameter> param) { bindParameter(param); }
    void setLabel(const std::string& label) { m_labelElement->setText(label); }

    void getPreferredSize(float& w, float& h) const override {
        static constexpr float KNOB_W = 80.0f; 
        static constexpr float KNOB_CIRCLE_H = 50.0f; 
        
        float lw = 0, lh = 0;
        m_labelElement->setWrapWidth(KNOB_W - 4.0f); 
        m_labelElement->getPreferredSize(lw, lh);
        
        w = KNOB_W;
        h = KNOB_CIRCLE_H + (lh > 0 ? lh + 8.0f : 0.0f);
    }

    void resized() override {
        static constexpr float KNOB_CIRCLE_H = 50.0f;
        float lw = 0, lh = 0;
        m_labelElement->getPreferredSize(lw, lh);
        
        m_labelElement->setBounds(0, KNOB_CIRCLE_H + 2.0f, m_bounds.w, lh);
        m_labelElement->resized();
    }

    void setTexture(std::shared_ptr<Texture> texture, int numFrames) {
        m_texture = texture;
        m_numFrames = numFrames;
    }

    void paint(QuadBatcher& g) override;

    std::string getTooltipText() const override {
        char valStr[64];
        snprintf(valStr, 64, "%s: %.2f", m_labelElement->getText().c_str(), getValue());
        return std::string(valStr);
    }

    void mouseDown(const MouseEvent& event) override {
        m_lastY = event.y;
        m_undoValue = getValue();
    }

    void mouseUp(const MouseEvent& event) override {
        float finalValue = getValue();
        if (m_parameter && std::abs(finalValue - m_undoValue) > 0.0001f) {
            UndoManager::get().perform(std::make_unique<ParameterChangeCommand>(m_parameter, m_undoValue, finalValue));
        }
    }

    void mouseDrag(const MouseEvent& event) override {
        float deltaY = m_lastY - event.y;
        float range = m_max - m_min;
        float sensitivity = event.shiftDown ? 0.0005f : 0.005f;
        
        m_value += deltaY * range * sensitivity;
        m_value = std::clamp(m_value, m_min, m_max);
        
        if (m_parameter) m_parameter->setValue(m_value);
        if (onValueChanged) onValueChanged(m_value);
        
        m_lastY = event.y;
    }

    float getValue() const { 
        if (m_parameter) return m_parameter->getValue();
        return m_value; 
    }
    
    void setValue(float v) { 
        m_value = std::clamp(v, m_min, m_max); 
        if (m_parameter) m_parameter->setValue(m_value);
    }

    float getMin() const { return m_min; }
    float getMax() const { return m_max; }
    const std::string& getLabel() const { return m_labelElement->getText(); }
    std::shared_ptr<Texture> getTexture() const { return m_texture; }
    int getNumFrames() const { return m_numFrames; }

    void setStyle(Theme::KnobStyle style) { m_style = style; }
    Theme::KnobStyle getStyle() const { return m_style; }

    std::shared_ptr<TextElement> getLabelElement() { return m_labelElement; }

    std::function<void(float)> onValueChanged;

private:
    std::shared_ptr<TextElement> m_labelElement;
    float m_min, m_max, m_value;
    float m_lastY = 0;
    float m_undoValue = 0;
    std::shared_ptr<Parameter> m_parameter;
    std::shared_ptr<Texture> m_texture;
    int m_numFrames = 0;
    Theme::KnobStyle m_style = Theme::KnobStyle::ClassicBakelite;
};

} // namespace Beam

#endif // KNOB_HPP
