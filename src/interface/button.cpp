#include "button.hpp"
#include "look_and_feel.hpp"
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
    auto& lf = getLookAndFeel();
    
    lf.drawButtonBackground(g, *this, m_isOver, m_isDown);
    lf.drawButtonText(g, *this, m_isOver, m_isDown);
    
    // Call base paint method if there's a callback
    Component::paint(g);
}

void Button::mouseDown(const MouseEvent& event) {
    if (m_enabled) {
        m_isDown = true;
        // Would trigger a repaint in a real implementation
    }
}

void Button::setToggleState(bool shouldBeOn, bool sendNotification) {
    if (m_toggleState != shouldBeOn) {
        m_toggleState = shouldBeOn;
        if (sendNotification && m_clickCallback) {
            m_clickCallback();
        }
    }
}

void Button::mouseUp(const MouseEvent& event) {
    if (m_enabled && m_isDown) {
        m_isDown = false;
        
        if (m_isToggle) {
            setToggleState(!m_toggleState, true);
        } else if (m_clickCallback) {
            m_clickCallback();
        }
    }
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





