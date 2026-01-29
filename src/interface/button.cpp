#include "button.hpp"
#include "../utilities/flux_audio_utils.hpp"

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
    // Auto-size based on text
    float tw = AudioUtils::calculateTextWidth(m_text, 14.0f);
    auto b = getBounds();
    if (b.w < tw + 20.0f) {
        setBounds(b.x, b.y, tw + 20.0f, b.h);
    }
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
    g.drawRoundedRect(bounds.x, bounds.y, bounds.w, bounds.h, 4.0f, 0.5f, r, g_color, b, 1.0f);

    // Draw text centered
    float tw = AudioUtils::calculateTextWidth(m_text, 14.0f);
    float textX = bounds.x + (bounds.w - tw) / 2.0f;
    float textY = bounds.y + (bounds.h - 14.0f) / 2.0f;
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





