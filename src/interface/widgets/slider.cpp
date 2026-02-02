#include "interface/widgets/slider.hpp"
#include "interface/core/look_and_feel.hpp"
#include <algorithm>

namespace Beam {

Slider::Slider() {
    setName("Slider");
}

Slider::Slider(std::shared_ptr<Parameter> parameter) : m_parameter(parameter) {
    if (m_parameter) {
        m_min = m_parameter->getMin();
        m_max = m_parameter->getMax();
        m_value = m_parameter->getValue();
    }
    setName("Slider");
}

Slider::~Slider() {
}

void Slider::setSliderStyle(SliderStyle style) {
    m_style = style;
}

void Slider::setRange(double min, double max, double interval) {
    m_min = min;
    m_max = max;
    m_interval = interval;
}

void Slider::setValue(double newValue, bool notify) {
    // Clamp to range
    newValue = (newValue < m_min) ? m_min : ((newValue > m_max) ? m_max : newValue);

    // Apply interval if specified
    if (m_interval > 0.0) {
        newValue = std::round((newValue - m_min) / m_interval) * m_interval + m_min;
    }

    m_value = newValue;

    if (m_parameter && notify) {
        m_parameter->setValue(static_cast<float>(newValue));
    }
}

double Slider::getValue() const {
    if (m_parameter) {
        return m_parameter->getValue();
    }
    return m_value;
}

void Slider::setTextValueSuffix(const std::string& suffix) {
    m_textSuffix = suffix;
}

void Slider::paint(QuadBatcher& g) {
    auto& lf = getLookAndFeel();
    
    float valueNorm = static_cast<float>((getValue() - m_min) / (m_max - m_min));
    float rotaryStartAngle = -135.0f;
    float rotaryEndAngle = 135.0f;

    lf.drawSliderBackground(g, *this, valueNorm, rotaryStartAngle, rotaryEndAngle);
    lf.drawSliderPointer(g, *this, valueNorm, rotaryStartAngle, rotaryEndAngle);
    
    // Call base paint method if there's a callback
    Component::paint(g);
}

static double getSliderValueFromPos(float x, float y, const Rect& bounds, SliderStyle style, double min, double max) {
    switch (style) {
        case SliderStyle::LinearHorizontal: {
            float margin = 6.0f; // Half cap width
            float effectiveW = bounds.w - 2 * margin;
            if (effectiveW < 1.0f) effectiveW = 1.0f;
            float norm = (x - margin) / effectiveW;
            return min + std::clamp(norm, 0.0f, 1.0f) * (max - min);
        }
        case SliderStyle::LinearVertical: {
            float margin = 14.0f; // Half cap height
            float effectiveH = bounds.h - 2 * margin;
            if (effectiveH < 1.0f) effectiveH = 1.0f;
            float norm = 1.0f - (y - margin) / effectiveH;
            return min + std::clamp(norm, 0.0f, 1.0f) * (max - min);
        }
        case SliderStyle::Rotary:
            return min + std::clamp(x / bounds.w, 0.0f, 1.0f) * (max - min);
    }
    return min;
}

void Slider::mouseDown(const MouseEvent& event) {
    m_dragStartX = event.x;
    m_dragStartY = event.y;
    m_dragStartValue = getValue();
    
    auto bounds = getBounds();
    
    if (!event.shiftDown && m_style != SliderStyle::Rotary) {
        // Absolute Jump
        double newVal = getSliderValueFromPos(event.x, event.y, bounds, m_style, m_min, m_max);
        setValue(newVal);
        m_dragStartValue = newVal; // Sync for relative drag
    }
}

void Slider::mouseDrag(const MouseEvent& event) {
    auto bounds = getBounds();
    
    if (!event.shiftDown && m_style != SliderStyle::Rotary) {
        // Absolute Tracking
        setValue(getSliderValueFromPos(event.x, event.y, bounds, m_style, m_min, m_max));
    } else {
        // Relative Drag
        float sensitivity = event.shiftDown ? 0.1f : 1.0f;
        double newValue = m_dragStartValue;
        
        switch (m_style) {
            case SliderStyle::LinearHorizontal: {
                float margin = 6.0f;
                float effectiveW = bounds.w - 2 * margin;
                if (effectiveW < 1.0f) effectiveW = 1.0f;
                float deltaX = (event.x - m_dragStartX) * sensitivity;
                newValue = m_dragStartValue + (deltaX / effectiveW) * (m_max - m_min);
                break;
            }
            case SliderStyle::LinearVertical: {
                float margin = 14.0f;
                float effectiveH = bounds.h - 2 * margin;
                if (effectiveH < 1.0f) effectiveH = 1.0f;
                float deltaY = (event.y - m_dragStartY) * sensitivity;
                newValue = m_dragStartValue - (deltaY / effectiveH) * (m_max - m_min); 
                break;
            }
            case SliderStyle::Rotary: {
                float deltaX = (event.x - m_dragStartX) * sensitivity;
                newValue = m_dragStartValue + (deltaX / bounds.w) * (m_max - m_min);
                break;
            }
        }
        setValue(newValue);
    }
}

void Slider::mouseUp(const MouseEvent& event) {
}

void Slider::setParameter(std::shared_ptr<Parameter> parameter) {
    m_parameter = parameter;
    if (m_parameter) {
        m_min = m_parameter->getMin();
        m_max = m_parameter->getMax();
        m_value = m_parameter->getValue();
    }
}

} // namespace Beam





