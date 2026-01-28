#include "gui_component.hpp"

namespace Beam {

GuiComponent::GuiComponent() {
}

GuiComponent::~GuiComponent() {
}

void GuiComponent::setBounds(float x, float y, float width, float height) {
    m_bounds = {x, y, width, height};
    resized();
}

void GuiComponent::setVisible(bool shouldBeVisible) {
    m_visible = shouldBeVisible;
}

void GuiComponent::paint(QuadBatcher& g) {
    // Default implementation - derived classes should override
    if (m_paintCallback) {
        m_paintCallback();
    }
}

void GuiComponent::resized() {
    if (m_resizedCallback) {
        m_resizedCallback();
    }
}

void GuiComponent::mouseEnter(const MouseEvent& event) {
    // Default implementation does nothing
}

void GuiComponent::mouseExit(const MouseEvent& event) {
    // Default implementation does nothing
}

void GuiComponent::mouseDown(const MouseEvent& event) {
    // Default implementation does nothing
}

void GuiComponent::mouseUp(const MouseEvent& event) {
    // Default implementation does nothing
}

void GuiComponent::mouseMove(const MouseEvent& event) {
    // Default implementation does nothing
}

void GuiComponent::mouseDrag(const MouseEvent& event) {
    // Default implementation does nothing
}

bool GuiComponent::keyPressed(const KeyPress& key) {
    // Default implementation returns false (key not handled)
    return false;
}

void GuiComponent::addChildComponent(std::shared_ptr<GuiComponent> child) {
    if (child) {
        m_children.push_back(child);
    }
}

void GuiComponent::removeChildComponent(std::shared_ptr<GuiComponent> child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it);
    }
}

void GuiComponent::paintEntireComponent(QuadBatcher& g) {
    if (!m_visible) return;
    
    paint(g);
    
    for (auto& child : m_children) {
        child->paintEntireComponent(g);
    }
}

bool GuiComponent::contains(float x, float y) const {
    return m_bounds.contains(x, y);
}

} // namespace Beam
