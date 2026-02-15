#ifndef GUI_LABEL_HPP
#define GUI_LABEL_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
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
    
    void setColor(Color c) { m_textColor = c; }

    void setJustificationType(int justification) {
        m_justification = justification;
    }

    void getPreferredSize(float& w, float& h) const override {
        w = m_text.length() * (m_fontSize * 0.6f); 
        h = m_fontSize + 4.0f;
    }

    void paint(QuadBatcher& g) override {
        float textY = (m_bounds.h - m_fontSize) * 0.5f;
        g.drawVectorText(m_text, 2, textY, m_fontSize, m_textColor.r, m_textColor.g, m_textColor.b, m_textColor.a);
    }

private:
    std::string m_text;
    float m_fontSize = 14.0f;
    int m_justification = 0;
    Color m_textColor = {0.9f, 0.9f, 0.9f, 1.0f};
};

} // namespace Beam

#endif // GUI_LABEL_HPP