#include "component.hpp"
#include "look_and_feel.hpp"
#include <iostream>

namespace Beam {

static DefaultLookAndFeel globalDefaultLookAndFeel;

Component::Component() {}
Component::~Component() {}

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
    m_bounds = {x, y, w, h};
    resized();
}

void Component::render(QuadBatcher& batcher, float dt, float screenW, float screenH) {
    if (!m_isVisible) return;

    // Clip to this component's bounds to prevent drawing outside/overlaps
    if (m_clipsChildren) {
        batcher.pushClip(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, screenH);
    }

    paint(batcher);

    for (auto& child : m_children) {
        child->render(batcher, dt, screenW, screenH);
    }

    if (m_clipsChildren) {
        batcher.popClip(screenH);
    }
}

bool Component::onMouseDown(float x, float y, int button) {
    if (!m_isVisible) return false;

    // Recursive hit test on children
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->getBounds().contains(x, y)) {
            // Check if child intercepts clicks (handled inside child's onMouseDown, 
            // but we need to call it to find out)
            if ((*it)->onMouseDown(x, y, button)) return true;
        }
    }

    // Self hit test
    if (m_interceptsMouseClicks && m_bounds.contains(x, y)) {
        if (m_isDraggable) startDragging(x, y);
        mouseDown(MouseEvent(x, y, button));
        return true;
    }

    return false;
}

bool Component::onMouseUp(float x, float y, int button) {
    if (!m_isVisible) return false;

    m_isDragging = false;

    bool handled = false;
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->onMouseUp(x, y, button)) handled = true;
    }

    mouseUp(MouseEvent(x, y, button));
    return true;
}

bool Component::onMouseMove(float x, float y) {
    if (!m_isVisible) return false;

    if (m_isDragging) {
        setBounds(x - m_dragStartX, y - m_dragStartY, m_bounds.w, m_bounds.h);
        mouseDrag(MouseEvent(x, y, 0));
        return true;
    }

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->onMouseMove(x, y)) return true;
    }

    mouseMove(MouseEvent(x, y, 0));
    m_lastMouseX = x;
    m_lastMouseY = y;
    return true;
}

bool Component::onMouseWheel(float x, float y, float delta) {
    if (!m_isVisible) return false;

    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if ((*it)->getBounds().contains(x, y)) {
            if ((*it)->onMouseWheel(x, y, delta)) return true;
        }
    }
    return false;
}

LookAndFeel& Component::getLookAndFeel() const {
    if (m_lookAndFeel) return *m_lookAndFeel;
    return globalDefaultLookAndFeel;
}

} // namespace Beam
