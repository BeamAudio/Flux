#include "interface/core/input_handler.hpp"
#include <algorithm>

namespace Beam {

void InputHandler::addComponent(std::shared_ptr<Component> component) {
    m_components.push_back(component);
}

bool InputHandler::handleMouseDown(float x, float y, int button, bool shift) {
    for (int i = (int)m_components.size() - 1; i >= 0; --i) {
        if (i >= (int)m_components.size()) continue;
        auto comp = m_components[i];
        if (comp->onMouseDown(x, y, button, shift)) {
             m_capturedComponent = comp;
             return true;
        }
    }
    return false;
}

bool InputHandler::handleMouseUp(float x, float y, int button, bool shift) {
    if (m_capturedComponent) {
        bool result = m_capturedComponent->onMouseUp(x, y, button, shift);
        m_capturedComponent = nullptr;
        return result; 
    }
    // Fallback: iterate (just in case)
    for (int i = (int)m_components.size() - 1; i >= 0; --i) {
        if (i >= (int)m_components.size()) continue;
        m_components[i]->onMouseUp(x, y, button, shift);
    }
    return true;
}

bool InputHandler::handleMouseMove(float x, float y, bool shift) {
    // 1. Reset hovered component
    m_hoveredComponent = nullptr;

    auto findHovered = [&](auto& self, Component* parent, float lx, float ly) -> Component* {
        if (!parent || !parent->isVisible()) return nullptr;
        
        // Check children in reverse order (top to bottom)
        const auto& children = parent->getChildren();
        for (int i = (int)children.size() - 1; i >= 0; --i) {
            auto child = children[i].get();
            if (child->isVisible() && child->getBounds().contains(lx, ly)) {
                Component* found = self(self, child, lx - child->getX(), ly - child->getY());
                if (found) return found;
                return child;
            }
        }
        return nullptr;
    };

    // Check all top-level components
    for (int i = (int)m_components.size() - 1; i >= 0; --i) {
        auto comp = m_components[i].get();
        if (comp->isVisible() && comp->getBounds().contains(x, y)) {
            m_hoveredComponent = findHovered(findHovered, comp, x - comp->getX(), y - comp->getY());
            if (!m_hoveredComponent) m_hoveredComponent = comp;
            break;
        }
    }

    if (m_capturedComponent) {
        return m_capturedComponent->onMouseMove(x, y, shift);
    }
    for (int i = (int)m_components.size() - 1; i >= 0; --i) {
        if (i >= (int)m_components.size()) continue;
        m_components[i]->onMouseMove(x, y, shift);
    }
    return true;
}

bool InputHandler::handleMouseWheel(float x, float y, float delta, bool shift) {
    for (int i = (int)m_components.size() - 1; i >= 0; --i) {
        if (i >= (int)m_components.size()) continue;
        if (m_components[i]->onMouseWheel(x, y, delta, shift)) return true;
    }
    return false;
}

void InputHandler::update(float dt) {
    for (auto& comp : m_components) {
        comp->update(dt);
    }
}

void InputHandler::render(QuadBatcher& batcher, float dt, float screenW, float screenH) {
    for (auto& comp : m_components) {
        comp->render(batcher, dt, screenW, screenH);
    }
}

} // namespace Beam





