#include "button.hpp"

namespace Beam {

Button::Button() : m_text("Button") {
    setName("Button");
}

Button::Button(const std::string& buttonText) : m_text(buttonText) {
    setName("Button");
}

Button::~Button() {
}

void Button::setButtonText(const std::string& newText) {
    m_text = newText;
}

void Button::setEnabled(bool shouldBeEnabled) {
    m_enabled = shouldBeEnabled;
}

void Button::onClick(std::function<void()> callback) {
    m_clickCallback = callback;
}

void Button::paint(QuadBatcher& g) {
    auto bounds = getBounds();
    
    // Determine colors based on state
    float r = 0.3f, g_color = 0.3f, b = 0.3f; // Default
    
    if (!m_enabled) {
        r = 0.2f; g_color = 0.2f; b = 0.2f; // Disabled
    } else if (m_isDown) {
        r = 0.2f; g_color = 0.4f; b = 0.6f; // Pressed
    } else if (m_isOver) {
        r = 0.4f; g_color = 0.6f; b = 0.8f; // Hover
    } else {
        r = 0.3f; g_color = 0.5f; b = 0.7f; // Normal
    }
    
    // Draw button background
    g.drawQuad(bounds.x, bounds.y, bounds.w, bounds.h, r, g_color, b, 1.0f);

    // Draw button border
    g.drawRect(bounds.x, bounds.y, bounds.w, bounds.h, 2.0f, 0.1f, 0.1f, 0.1f, 1.0f);
    
    // Draw text (simplified - in a real implementation, we'd use proper text rendering)
    // For now, we'll just draw a small indicator in the center
    float textX = bounds.x + bounds.w/2 - 15;
    float textY = bounds.y + bounds.h/2 + 5;
    g.drawText(m_text, textX, textY, 14.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    
    // Call base paint method if there's a callback
    GuiComponent::paint(g);
}

void Button::mouseDown(const MouseEvent& event) {
    if (m_enabled) {
        m_isDown = true;
        // Would trigger a repaint in a real implementation
    }
}

void Button::mouseUp(const MouseEvent& event) {
    if (m_enabled && m_isDown) {
        m_isDown = false;
        if (m_clickCallback) {
            m_clickCallback();
        }
    }
    // Would trigger a repaint in a real implementation
}

void Button::mouseEnter(const MouseEvent& event) {
    if (m_enabled) {
        m_isOver = true;
        // Would trigger a repaint in a real implementation
    }
}

void Button::mouseExit(const MouseEvent& event) {
    m_isOver = false;
    m_isDown = false;
    // Would trigger a repaint in a real implementation
}

} // namespace Beam





