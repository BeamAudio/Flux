#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "../render/quad_batcher.hpp"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <iostream>

namespace Beam {

struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class MouseEvent {
public:
    float x, y;
    int button;
    MouseEvent(float x_pos, float y_pos, int btn) : x(x_pos), y(y_pos), button(btn) {}
};

class LookAndFeel;

/**
 * @class Component
 * @brief The base class for all UI elements in Beam Audio Flux.
 * Inspired by JUCE's Component architecture.
 */
class Component : public std::enable_shared_from_this<Component> {
public:
    Component();
    virtual ~Component();

    // --- Lifecycle & Hierarchy ---
    virtual void setName(const std::string& name) { m_name = name; }
    virtual const std::string& getName() const { return m_name; }

    void addChildComponent(std::shared_ptr<Component> child);
    void removeChildComponent(Component* child);
    const std::vector<std::shared_ptr<Component>>& getChildren() const { return m_children; }
    
    Component* getParent() const { return m_parent; }
    void setParent(Component* parent) { m_parent = parent; }

    template <typename T>
    T* findParent() {
        Component* p = m_parent;
        while (p) {
            if (T* t = dynamic_cast<T*>(p)) return t;
            p = p->getParent();
        }
        return nullptr;
    }

    Rect getScreenBounds() const {
        float x = m_bounds.x;
        float y = m_bounds.y;
        Component* p = m_parent;
        while (p) {
            x += p->getX();
            y += p->getY();
            p = p->getParent();
        }
        return {x, y, m_bounds.w, m_bounds.h};
    }

    // --- Layout ---
    virtual void setBounds(float x, float y, float w, float h);
    virtual void setBounds(Rect newBounds) { setBounds(newBounds.x, newBounds.y, newBounds.w, newBounds.h); }
    Rect getBounds() const { return m_bounds; }
    float getWidth() const { return m_bounds.w; }
    float getHeight() const { return m_bounds.h; }
    float getX() const { return m_bounds.x; }
    float getY() const { return m_bounds.y; }

    virtual void resized() {}

    // --- Rendering ---
    /**
     * @brief High-level render call. Handles visibility and recursion.
     * @note Do not override unless you need custom recursion logic.
     */
    virtual void render(QuadBatcher& batcher, float dt, float screenW, float screenH);

    /**
     * @brief The main painting method for subclasses.
     */
    virtual void paint(QuadBatcher& g) {}

    virtual void setVisible(bool visible) { m_isVisible = visible; }
    virtual bool isVisible() const { return m_isVisible; }

    // --- Interaction ---
    /**
     * @brief Sets whether this component should intercept mouse clicks.
     * @param intercepts If false, clicks pass through to components behind it.
     */
    void setInterceptsMouseClicks(bool intercepts) { m_interceptsMouseClicks = intercepts; }
    bool getInterceptsMouseClicks() const { return m_interceptsMouseClicks; }

    virtual bool onMouseDown(float x, float y, int button);
    virtual bool onMouseUp(float x, float y, int button);
    virtual bool onMouseMove(float x, float y);
    virtual bool onMouseWheel(float x, float y, float delta);

    virtual void mouseDown(const MouseEvent& event) {}
    virtual void mouseUp(const MouseEvent& event) {}
    virtual void mouseMove(const MouseEvent& event) {}
    virtual void mouseEnter(const MouseEvent& event) {}
    virtual void mouseExit(const MouseEvent& event) {}
    virtual void mouseDrag(const MouseEvent& event) {}
    virtual void update(float dt) {
        for (auto& child : m_children) child->update(dt);
    }

    void setDraggable(bool draggable) { m_isDraggable = draggable; }
    void setClipsChildren(bool clip) { m_clipsChildren = clip; }
    
    void startDragging(float x, float y) {
        m_isDragging = true;
        m_dragStartX = x - m_bounds.x;
        m_dragStartY = y - m_bounds.y;
    }

    // --- Look & Feel ---
    void setLookAndFeel(LookAndFeel* lf) { m_lookAndFeel = lf; }
    LookAndFeel& getLookAndFeel() const;

protected:
    Rect m_bounds = {0, 0, 0, 0};
    bool m_isVisible = true;
    bool m_isDraggable = false;
    bool m_isDragging = false;
    bool m_interceptsMouseClicks = true;
    bool m_clipsChildren = true;
    float m_dragStartX = 0, m_dragStartY = 0;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    std::string m_name;

    std::vector<std::shared_ptr<Component>> m_children;
    Component* m_parent = nullptr;
    LookAndFeel* m_lookAndFeel = nullptr;
};

} // namespace Beam

#endif