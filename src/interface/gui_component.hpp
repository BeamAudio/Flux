#ifndef GUI_COMPONENT_HPP
#define GUI_COMPONENT_HPP

#include "../render/quad_batcher.hpp"
#include "../interface/component.hpp"  // Include the existing component header that has Rect
#include <vector>
#include <functional>
#include <memory>
#include <string>

namespace Beam {

class MouseEvent {
public:
    float x, y;
    int eventNumber;
    bool mouseWasDragged;
    
    MouseEvent(float x_pos, float y_pos, int event_num, bool dragged = false) 
        : x(x_pos), y(y_pos), eventNumber(event_num), mouseWasDragged(dragged) {}
};

class KeyPress {
public:
    int keyCode;
    std::string keyText;
    
    KeyPress(int code, const std::string& text) : keyCode(code), keyText(text) {}
};

/**
 * @class GuiComponent
 * @brief Base class for GUI components, similar to JUCE's Component
 */
class GuiComponent {
public:
    GuiComponent();
    virtual ~GuiComponent();

    /**
     * @brief Sets the bounds of this component
     */
    virtual void setBounds(float x, float y, float width, float height);

    /**
     * @brief Gets the bounds of this component
     */
    const Rect& getBounds() const { return m_bounds; }

    /**
     * @brief Sets the component's visibility
     */
    void setVisible(bool shouldBeVisible);

    /**
     * @brief Checks if the component is visible
     */
    bool isVisible() const { return m_visible; }

    /**
     * @brief Called when the component needs to be painted
     */
    virtual void paint(QuadBatcher& g);

    /**
     * @brief Called when the component's size changes
     */
    virtual void resized();

    /**
     * @brief Called when the mouse enters the component
     */
    virtual void mouseEnter(const MouseEvent& event);

    /**
     * @brief Called when the mouse exits the component
     */
    virtual void mouseExit(const MouseEvent& event);

    /**
     * @brief Called when a mouse button is pressed
     */
    virtual void mouseDown(const MouseEvent& event);

    /**
     * @brief Called when a mouse button is released
     */
    virtual void mouseUp(const MouseEvent& event);

    /**
     * @brief Called when the mouse is moved
     */
    virtual void mouseMove(const MouseEvent& event);

    /**
     * @brief Called when the mouse is dragged
     */
    virtual void mouseDrag(const MouseEvent& event);

    /**
     * @brief Called when a key is pressed
     */
    virtual bool keyPressed(const KeyPress& key);

    /**
     * @brief Adds a child component
     */
    void addChildComponent(std::shared_ptr<GuiComponent> child);

    /**
     * @brief Removes a child component
     */
    void removeChildComponent(std::shared_ptr<GuiComponent> child);

    /**
     * @brief Paints this component and all its children
     */
    void paintEntireComponent(QuadBatcher& g);

    /**
     * @brief Checks if a point is inside this component
     */
    bool contains(float x, float y) const;

    /**
     * @brief Sets the component's name
     */
    void setName(const std::string& name) { m_name = name; }

    /**
     * @brief Gets the component's name
     */
    const std::string& getName() const { return m_name; }

protected:
    Rect m_bounds{0, 0, 0, 0};
    bool m_visible = true;
    std::vector<std::shared_ptr<GuiComponent>> m_children;
    std::string m_name;

    std::function<void()> m_paintCallback;
    std::function<void()> m_resizedCallback;
};

} // namespace Beam

#endif // GUI_COMPONENT_HPP
