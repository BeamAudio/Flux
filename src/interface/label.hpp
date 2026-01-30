#ifndef GUI_LABEL_HPP
#define GUI_LABEL_HPP

#include "component.hpp"
#include <string>

namespace Beam {

/**
 * @class Label
 * @brief A simple text label component
 */
class Label : public Component {
public:
    Label(const std::string& text = "") : m_text(text) {
        setName("Label");
    }

    void setText(const std::string& newText) {
        m_text = newText;
    }

    const std::string& getText() const {
        return m_text;
    }

    void setFontSize(float size) {
        m_fontSize = size;
    }

    float getFontSize() const {
        return m_fontSize;
    }

    void setJustificationType(int justification) {
        m_justification = justification;
    }

    void paint(QuadBatcher& g) override {
        auto bounds = getBounds();
        // Justification logic could be more complex, but for now:
        g.drawText(m_text, bounds.x, bounds.y, m_fontSize, 0.9f, 0.9f, 0.9f, 1.0f);
        
        GuiComponent::paint(g);
    }

private:
    std::string m_text;
    float m_fontSize = 14.0f;
    int m_justification = 0;
};

} // namespace Beam

#endif // GUI_LABEL_HPP
