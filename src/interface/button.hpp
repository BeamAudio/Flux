#ifndef GUI_BUTTON_HPP
#define GUI_BUTTON_HPP

#include "component.hpp"
#include <functional>
#include <string>

namespace Beam {

/**
 * @class Button
 * @brief A button component, similar to JUCE's Button
 */
class Button : public Component {
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
     * @brief Checks if the mouse is over the button
     */
    bool isMouseOver() const { return m_isOver; }

    /**
     * @brief Checks if the button is currently being pressed
     */
    bool isMouseButtonDown() const { return m_isDown; }

    /**
     * @brief Sets the toggle state of the button
     */
    void setToggleState(bool shouldBeOn, bool sendNotification = true);

    /**
     * @brief Gets the toggle state
     */
    bool getToggleState() const { return m_toggleState; }

    /**
     * @brief Sets whether this button acts as a toggle
     */
    void setClickingTogglesState(bool shouldToggle) { m_isToggle = shouldToggle; }

    /**
     * @brief Checks if this is a toggle button
     */
    bool getClickingTogglesState() const { return m_isToggle; }

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
    bool m_toggleState = false;
    bool m_isToggle = false;
    std::function<void()> m_clickCallback;
};

/**
 * @class TextButton
 * @brief A standard text button
 */
class TextButton : public Button {
public:
    explicit TextButton(const std::string& text = "Button") : Button(text) {}
};

/**
 * @class ToggleButton
 * @brief A button that stays in a toggled state
 */
class ToggleButton : public Button {
public:
    explicit ToggleButton(const std::string& text = "Toggle") : Button(text) {
        setClickingTogglesState(true);
    }
};

} // namespace Beam

#endif // GUI_BUTTON_HPP





