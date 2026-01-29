#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <vector>
#include <memory>
#include <string>

namespace Beam {

class QuadBatcher; // Forward declaration

struct Rect {
    float x, y, w, h;
    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

class Component {
public:
    virtual ~Component() = default;

    virtual void update(float dt) {}
    virtual void render(QuadBatcher& batcher, float dt) = 0;

    virtual bool onMouseDown(float x, float y, int button) { return false; }
    virtual bool onMouseUp(float x, float y, int button) { 
        m_isDragging = false; 
        return false; 
    }
    virtual bool onMouseMove(float x, float y) { 
        if (m_isDragging && m_isDraggable) {
            float dx = x - m_lastMouseX;
            float dy = y - m_lastMouseY;
            setBounds(m_bounds.x + dx, m_bounds.y + dy, m_bounds.w, m_bounds.h);
            m_lastMouseX = x;
            m_lastMouseY = y;
            return true;
        }
        return false; 
    }

    virtual void setBounds(float x, float y, float w, float h) { m_bounds = {x, y, w, h}; }
    const Rect& getBounds() const { return m_bounds; }
    
    void setDraggable(bool draggable) { m_isDraggable = draggable; }
    void startDragging(float x, float y) { 
        m_isDragging = true; 
        m_lastMouseX = x; 
        m_lastMouseY = y; 
    }

protected:
    Rect m_bounds{0, 0, 0, 0};
    bool m_isVisible = true;
    bool m_isEnabled = true;
    bool m_isDraggable = false;
    bool m_isDragging = false;
    float m_lastMouseX = 0, m_lastMouseY = 0;
};

} // namespace Beam

#endif // COMPONENT_HPP



