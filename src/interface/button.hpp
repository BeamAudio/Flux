#ifndef GUI_BUTTON_HPP
#define GUI_BUTTON_HPP

#include "gui_component.hpp"
#include <functional>
#include <string>

namespace Beam {

/**
 * @class Button
 * @brief A button component, similar to JUCE's Button
 */
class Button : public GuiComponent {
public:
    Button();
    explicit Button(const std::string& buttonText);
    ~Button() override;

    /**
     * @brief Sets the button text
     */
    void setButtonText(const std::string& newText);

    /**
     * @brief Gets the button text
     */
    const std::string& getButtonText() const { return m_text; }

    /**
     * @brief Sets whether the button is clickable
     */
    void setEnabled(bool shouldBeEnabled);

    /**
     * @brief Checks if the button is enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Sets the click listener
     */
    void onClick(std::function<void()> callback);

    /**
     * @brief Paints the button
     */
    void paint(QuadBatcher& g) override;

    /**
     * @brief Called when the mouse is pressed
     */
    void mouseDown(const MouseEvent& event) override;

    /**
     * @brief Called when the mouse is released
     */
    void mouseUp(const MouseEvent& event) override;

    /**
     * @brief Called when the mouse enters the component
     */
    void mouseEnter(const MouseEvent& event) override;

    /**
     * @brief Called when the mouse exits the component
     */
    void mouseExit(const MouseEvent& event) override;

private:
    std::string m_text;
    bool m_enabled = true;
    bool m_isOver = false;
    bool m_isDown = false;
    std::function<void()> m_clickCallback;
};

} // namespace Beam

#endif // GUI_BUTTON_HPP
