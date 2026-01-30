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

/**
 * @class Knob
 * @brief A rotary knob component
 */
class Knob : public Component {
public:
    Knob(const std::string& label, float minVal, float maxVal, float initialVal)
        : m_label(label), m_min(minVal), m_max(maxVal), m_value(initialVal) {
        setName("Knob");
    }

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

    void paint(QuadBatcher& batcher) override;

    void mouseDown(const MouseEvent& event) override {
        m_isDragging = true;
        m_lastY = event.y;
    }

    void mouseUp(const MouseEvent& event) override {
        m_isDragging = false;
    }

    void mouseDrag(const MouseEvent& event) override {
        if (m_isDragging) {
            float deltaY = m_lastY - event.y;
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
            
            m_lastY = event.y;
        }
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
    const std::string& getLabel() const { return m_label; }
    std::shared_ptr<Texture> getTexture() const { return m_texture; }
    int getNumFrames() const { return m_numFrames; }

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