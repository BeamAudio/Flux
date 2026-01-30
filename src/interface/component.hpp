#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "../render/quad_batcher.hpp"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

namespace Beam {

struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class Component {
public:
    Component() = default;
    virtual ~Component() = default;

    void setName(const std::string& name) { m_name = name; }
    const std::string& getName() const { return m_name; }

    virtual void render(QuadBatcher& batcher, float dt, float screenW, float screenH) {}
    virtual void update(float dt) {}

    virtual bool onMouseDown(float x, float y, int button) {
        if (m_isDraggable && m_bounds.contains(x, y)) {
            startDragging(x, y);
            return true;
        }
        return false;
    }

    void startDragging(float x, float y) {
        m_isDragging = true;
        m_dragStartX = x - m_bounds.x;
        m_dragStartY = y - m_bounds.y;
    }

    virtual bool onMouseUp(float x, float y, int button) {
        m_isDragging = false;
        return false;
    }

    virtual bool onMouseMove(float x, float y) {
        if (m_isDragging) {
            setBounds(x - m_dragStartX, y - m_dragStartY, m_bounds.w, m_bounds.h);
            return true;
        }
        m_lastMouseX = x;
        m_lastMouseY = y;
        return false;
    }

    virtual bool onMouseWheel(float x, float y, float delta) { return false; }

    virtual void setBounds(float x, float y, float w, float h) {
        m_bounds = {x, y, w, h};
    }

    Rect getBounds() const { return m_bounds; }
    void setDraggable(bool draggable) { m_isDraggable = draggable; }
    
    void setVisible(bool visible) { m_isVisible = visible; }
    bool isVisible() const { return m_isVisible; }

protected:
    Rect m_bounds = {0, 0, 0, 0};
    bool m_isDraggable = false;
    bool m_isDragging = false;
    bool m_isVisible = true;
    float m_dragStartX = 0, m_dragStartY = 0;
    float m_lastMouseX = 0, m_lastMouseY = 0;
    std::string m_name;
};

} // namespace Beam

#endif
