#include "input_handler.hpp"
#include <algorithm>

namespace Beam {

void InputHandler::addComponent(std::shared_ptr<Component> component) {
    m_components.push_back(component);
}

void InputHandler::handleMouseDown(float x, float y, int button) {
    for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
        if ((*it)->getBounds().contains(x, y)) {
            if ((*it)->onMouseDown(x, y, button)) {
                m_focusedComponent = *it;
                break;
            }
        }
    }
}

void InputHandler::handleMouseUp(float x, float y, int button) {
    if (m_focusedComponent) {
        m_focusedComponent->onMouseUp(x, y, button);
        m_focusedComponent = nullptr;
    }
}

void InputHandler::handleMouseMove(float x, float y) {
    if (m_focusedComponent) {
        m_focusedComponent->onMouseMove(x, y);
    } else {
        for (auto it = m_components.rbegin(); it != m_components.rend(); ++it) {
            if ((*it)->getBounds().contains(x, y)) {
                if ((*it)->onMouseMove(x, y)) break;
            }
        }
    }
}

void InputHandler::update(float dt) {
    for (auto& comp : m_components) {
        comp->update(dt);
    }
}

void InputHandler::render(QuadBatcher& batcher) {
    for (auto& comp : m_components) {
        comp->render(batcher);
    }
}

} // namespace Beam
