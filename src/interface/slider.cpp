#include "slider.hpp"
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
    auto bounds = getBounds();
    
    // Draw background
    g.drawQuad(bounds.x, bounds.y, bounds.w, bounds.h, 0.2f, 0.2f, 0.2f, 1.0f);

    // Draw slider fill based on style
    float valueNorm = static_cast<float>((getValue() - m_min) / (m_max - m_min));

    switch (m_style) {
        case SliderStyle::LinearHorizontal:
            g.drawQuad(bounds.x, bounds.y, bounds.w * valueNorm, bounds.h, 0.3f, 0.5f, 0.8f, 1.0f);
            break;

        case SliderStyle::LinearVertical:
            g.drawQuad(bounds.x, bounds.y + bounds.h * (1.0f - valueNorm), bounds.w, bounds.h * valueNorm, 0.3f, 0.5f, 0.8f, 1.0f);
            break;

        case SliderStyle::Rotary:
            // Simplified rotary visualization - draw a circle with indicator
            float centerX = bounds.x + bounds.w / 2.0f;
            float centerY = bounds.y + bounds.h / 2.0f;
            float radius = (bounds.w < bounds.h ? bounds.w : bounds.h) / 2.0f - 5.0f;

            // Draw circle
            g.drawQuad(centerX - radius, centerY - radius, radius * 2, radius * 2, 0.3f, 0.3f, 0.3f, 1.0f);

            // Draw indicator
            float angle = 270.0f + valueNorm * 270.0f; // From -90° to 180°
            float radAngle = angle * 3.14159f / 180.0f;
            float indicatorX = centerX + cosf(radAngle) * radius * 0.7f;
            float indicatorY = centerY + sinf(radAngle) * radius * 0.7f;

            g.drawQuad(indicatorX - 3, indicatorY - 3, 6, 6, 0.8f, 0.8f, 0.2f, 1.0f);
            break;
    }
    
    // Call base paint method if there's a callback
    GuiComponent::paint(g);
}

void Slider::mouseDown(const MouseEvent& event) {
    m_isDragging = true;
    m_dragStartX = event.x;
    m_dragStartY = event.y;
    m_dragStartValue = getValue();
    
    // Calculate new value based on click position
    auto bounds = getBounds();
    double newValue = 0.0;
    
    switch (m_style) {
        case SliderStyle::LinearHorizontal:
            newValue = m_min + (event.x - bounds.x) / bounds.w * (m_max - m_min);
            break;

        case SliderStyle::LinearVertical:
            newValue = m_min + (bounds.y + bounds.h - event.y) / bounds.h * (m_max - m_min);
            break;

        case SliderStyle::Rotary:
            // For rotary, we could calculate angle, but for simplicity, use horizontal mapping
            newValue = m_min + (event.x - bounds.x) / bounds.w * (m_max - m_min);
            break;
    }
    
    setValue(newValue);
}

void Slider::mouseDrag(const MouseEvent& event) {
    if (!m_isDragging) return;
    
    auto bounds = getBounds();
    double newValue = m_dragStartValue;
    
    switch (m_style) {
        case SliderStyle::LinearHorizontal:
            {
                float deltaX = event.x - m_dragStartX;
                newValue = m_dragStartValue + (deltaX / bounds.w) * (m_max - m_min);
            }
            break;
            
        case SliderStyle::LinearVertical:
            {
                float deltaY = event.y - m_dragStartY;
                newValue = m_dragStartValue - (deltaY / bounds.h) * (m_max - m_min); // Invert Y axis
            }
            break;
            
        case SliderStyle::Rotary:
            // For rotary, adjust based on X movement
            {
                float deltaX = event.x - m_dragStartX;
                newValue = m_dragStartValue + (deltaX / bounds.w) * (m_max - m_min);
            }
            break;
    }
    
    setValue(newValue);
}

void Slider::mouseUp(const MouseEvent& event) {
    m_isDragging = false;
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
