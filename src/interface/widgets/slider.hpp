#ifndef GUI_SLIDER_HPP
#define GUI_SLIDER_HPP

#include "interface/core/component.hpp"
#include "engine/session/parameter.hpp"
#include <memory>

namespace Beam {

/**
 * @enum SliderStyle
 * @brief Different styles of sliders
 */
enum class SliderStyle {
    LinearHorizontal,
    LinearVertical,
    Rotary
};

/**
 * @class Slider
 * @brief A slider component, similar to JUCE's Slider
 */
class Slider : public Component {
public:
    Slider();
    explicit Slider(std::shared_ptr<Parameter> parameter);
    ~Slider() override;

    /**
     * @brief Sets the slider style
     */
    void setSliderStyle(SliderStyle style);

    /**
     * @brief Gets the slider style
     */
    SliderStyle getSliderStyle() const { return m_style; }

    void getPreferredSize(float& w, float& h) const override {
        if (m_style == SliderStyle::LinearHorizontal) { w = 120; h = 24; }
        else if (m_style == SliderStyle::LinearVertical) { w = 24; h = 120; }
        else { w = 40; h = 40; } // Rotary
    }

    /**
     * @brief Sets the range of values
     */
    void setRange(double min, double max, double interval = 0.0);

    /**
     * @brief Sets the current value
     */
    void setValue(double newValue, bool notify = true);

    /**
     * @brief Gets the current value
     */
    double getValue() const;

    /**
     * @brief Sets the text value to display
     */
    void setTextValueSuffix(const std::string& suffix);

    /**
     * @brief Paints the slider
     */
    void paint(QuadBatcher& g) override;

    /**
     * @brief Called when the mouse is pressed
     */
    void mouseDown(const MouseEvent& event) override;

    /**
     * @brief Called when the mouse is dragged
     */
    void mouseDrag(const MouseEvent& event) override;

    /**
     * @brief Called when the mouse is released
     */
    void mouseUp(const MouseEvent& event) override;

    /**
     * @brief Sets the parameter to control
     */
    void setParameter(std::shared_ptr<Parameter> parameter);

    /**
     * @brief Gets the controlled parameter
     */
    std::shared_ptr<Parameter> getParameter() const { return m_parameter; }

private:
    SliderStyle m_style = SliderStyle::LinearHorizontal;
    double m_min = 0.0;
    double m_max = 1.0;
    double m_interval = 0.0;
    double m_value = 0.0;
    std::string m_textSuffix;
    std::shared_ptr<Parameter> m_parameter;
    float m_dragStartX, m_dragStartY;
    double m_dragStartValue;
};

} // namespace Beam

#endif // GUI_SLIDER_HPP





