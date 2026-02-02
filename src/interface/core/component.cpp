#include "interface/core/component.hpp"
#include "interface/core/look_and_feel.hpp"
#include <iostream>

namespace Beam {

static ModernLookAndFeel globalDefaultLookAndFeel;

Component::Component() {}
Component::~Component() {}

void Component::localToScreen(float& x, float& y) {
    // 1. Apply Local Position
    x += m_bounds.x;
    y += m_bounds.y;
    // 2. Delegate to Parent
    if (m_parent) {
        m_parent->localToScreen(x, y);
    }
}

void Component::screenToLocal(float& x, float& y) {
    // 1. Delegate to Parent (Reverse Order)
    if (m_parent) {
        m_parent->screenToLocal(x, y);
    }
    // 2. Apply Local Position (Reverse)
    x -= m_bounds.x;
    y -= m_bounds.y;
}

void Component::addChildComponent(std::shared_ptr<Component> child) {
    if (child) {
        child->m_parent = this;
        m_children.push_back(child);
    }
}

void Component::removeChildComponent(Component* child) {
    m_children.erase(std::remove_if(m_children.begin(), m_children.end(),
        [child](const std::shared_ptr<Component>& c) { return c.get() == child; }), m_children.end());
}

void Component::setBounds(float x, float y, float w, float h) {
    bool sizeChanged = (m_bounds.w != w || m_bounds.h != h);
    m_bounds = {x, y, w, h};
    if (sizeChanged) resized();
    if (m_parent) m_parent->childBoundsChanged(this);
}

void Component::render(QuadBatcher& batcher, float dt, float screenW, float screenH) {
    if (!m_isVisible) return;

    // 1. Push Offset for local drawing
    batcher.pushOffset(m_bounds.x, m_bounds.y);

    // 2. Clip to this component's bounds (using local coordinates)
    if (m_clipsChildren) {
        batcher.pushClip(0, 0, m_bounds.w, m_bounds.h, screenH);
    }

    // 3. Paint in local space (0,0 is top-left)
    paint(batcher);

    // 4. Render children
    for (auto& child : m_children) {
        child->render(batcher, dt, screenW, screenH);
    }

    if (m_clipsChildren) {
        batcher.popClip(screenH);
    }

    // 5. Pop Offset
    batcher.popOffset();
}

bool Component::onMouseDown(float x, float y, int button, bool shift) {
    if (!m_isVisible) return false;
    if (!m_bounds.contains(x, y)) return false;

    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    // 1. Give children a chance first - iterate backwards by index to be safe against removals
    for (int i = (int)m_children.size() - 1; i >= 0; --i) {
        // Ensure index is still valid (in case previous child removed siblings)
        if (i >= (int)m_children.size()) continue; 
        
        auto child = m_children[i];
        if (child->onMouseDown(localX, localY, button, shift)) {
            m_capturedChild = child; // Capture for subsequent Move/Up
            return true;
        }
    }

    // 2. If no child consumed it, handle locally
    if (m_isDraggable) startDragging(x, y);
    mouseDown(MouseEvent(localX, localY, button, shift));

    return true; 
}

bool Component::onMouseUp(float x, float y, int button, bool shift) {
    if (!m_isVisible) return false;
    
    m_isDragging = false;
    
    // We notify even if not 'contained' to ensure button releases etc.
    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    mouseUp(MouseEvent(localX, localY, button, shift));

    if (m_capturedChild) {
        float childX = localX - m_capturedChild->m_bounds.x;
        float childY = localY - m_capturedChild->m_bounds.y;
        m_capturedChild->onMouseUp(childX, childY, button, shift);
        m_capturedChild = nullptr; // Release capture
    } else {
        for (int i = (int)m_children.size() - 1; i >= 0; --i) {
            if (i >= (int)m_children.size()) continue;
            auto child = m_children[i];
            float childX = localX - child->m_bounds.x;
            float childY = localY - child->m_bounds.y;
            child->onMouseUp(childX, childY, button, shift);
        }
    }
    return true;
}

bool Component::onMouseMove(float x, float y, bool shift) {
    if (!m_isVisible) return false;

    if (m_isDragging) {
        // Dragging happens in parent space
        setBounds(x - m_dragStartX, y - m_dragStartY, m_bounds.w, m_bounds.h);
        mouseDrag(MouseEvent(x - m_bounds.x, y - m_bounds.y, 0, shift));
        return true;
    }

    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    if (m_capturedChild) {
        float childX = localX - m_capturedChild->m_bounds.x;
        float childY = localY - m_capturedChild->m_bounds.y;
        
        // During capture, call mouseDrag on the captured child (what widgets like Knob expect)
        m_capturedChild->mouseDrag(MouseEvent(childX, childY, 0, shift));
        // Also propagate onMouseMove so nested captures work
        m_capturedChild->onMouseMove(childX, childY, shift);
        return true;
    }

    bool hit = m_bounds.contains(x, y);
    if (hit) {
        mouseMove(MouseEvent(localX, localY, 0, shift));
    }

    // Default propagation (if no capture)
    for (int i = (int)m_children.size() - 1; i >= 0; --i) {
        if (i >= (int)m_children.size()) continue;
        auto child = m_children[i];
        float childX = localX - child->m_bounds.x;
        float childY = localY - child->m_bounds.y;
        child->onMouseMove(childX, childY, shift);
    }

    m_lastMouseX = localX;
    m_lastMouseY = localY;
    return hit;
}

bool Component::onMouseWheel(float x, float y, float delta, bool shift) {
    if (!m_isVisible || !m_bounds.contains(x, y)) return false;

    float localX = x - m_bounds.x;
    float localY = y - m_bounds.y;

    for (int i = (int)m_children.size() - 1; i >= 0; --i) {
        if (i >= (int)m_children.size()) continue;
        if (m_children[i]->onMouseWheel(localX, localY, delta, shift)) return true;
    }
    return true;
}

LookAndFeel& Component::getLookAndFeel() const {
    if (m_lookAndFeel) return *m_lookAndFeel;
    return globalDefaultLookAndFeel;
}

} // namespace Beam