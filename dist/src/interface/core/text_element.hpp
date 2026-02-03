#ifndef BEAM_TEXT_ELEMENT_HPP
#define BEAM_TEXT_ELEMENT_HPP

#include "interface/core/component.hpp"
#include "engine/dsp/flux_audio_utils.hpp"
#include <string>
#include <vector>

namespace Beam {

/**
 * @class TextElement
 * @brief A robust text component with wrapping, measurement, and justification.
 * Designed to integrate perfectly with AutoFlexContainer.
 */
class TextElement : public Component {
public:
    enum class Justification { Left, Center, Right };
    
    TextElement(const std::string& text = "") : m_text(text) {
        setName("TextElement");
    }
    
    void setText(const std::string& text) { m_text = text; }
    const std::string& getText() const { return m_text; }
    
    void setFontSize(float size) { m_fontSize = size; }
    float getFontSize() const { return m_fontSize; }
    
    void setColor(float r, float g, float b, float a = 1.0f) { m_color = {r, g, b, a}; }
    
    void setWrapWidth(float width) { m_wrapWidth = width; }
    void setLineHeight(float height) { m_lineHeight = height; }
    void setJustification(Justification j) { m_justification = j; }
    
    void getPreferredSize(float& w, float& h) const override {
        if (m_text.empty()) { w = 0; h = 0; return; }
        
        float targetWrap = m_wrapWidth > 0 ? m_wrapWidth : 200.0f;
        auto lines = AudioUtils::wrapText(m_text, m_fontSize, targetWrap);
        
        float maxW = 0;
        for (const auto& line : lines) {
            float lw = AudioUtils::calculateTextWidth(line, m_fontSize);
            if (lw > maxW) maxW = lw;
        }
        
        float rowH = m_lineHeight > 0 ? m_lineHeight : m_fontSize * 1.35f; // More vertical spacing
        w = maxW + 4.0f; // Add horizontal safety margin
        h = lines.size() * rowH + 2.0f; // Add vertical safety margin
    }
    
    void paint(QuadBatcher& batcher) override {
        if (m_text.empty()) return;
        
        float rowH = m_lineHeight > 0 ? m_lineHeight : m_fontSize * 1.35f;
        // Use a slightly tighter wrap in paint to stay inside bounds
        auto lines = AudioUtils::wrapText(m_text, m_fontSize, m_bounds.w - 2.0f);
        
        for (size_t i = 0; i < lines.size(); ++i) {
            float lw = AudioUtils::calculateTextWidth(lines[i], m_fontSize);
            float xOff = 1.0f; // 1px start padding
            
            if (m_justification == Justification::Center) xOff = (m_bounds.w - lw) * 0.5f;
            else if (m_justification == Justification::Right) xOff = (m_bounds.w - lw) - 1.0f;
            
            // Draw relative to component (render already pushed m_bounds.x/y)
            batcher.drawText(lines[i], xOff, i * rowH + 1.0f, 
                             m_fontSize, m_color.r, m_color.g, m_color.b, m_color.a);
        }
    }

private:
    std::string m_text;
    float m_fontSize = 12.0f;
    float m_wrapWidth = -1.0f;
    float m_lineHeight = -1.0f;
    Justification m_justification = Justification::Left;
    struct { float r, g, b, a; } m_color = {0.9f, 0.9f, 0.9f, 1.0f};
};

} // namespace Beam

#endif // BEAM_TEXT_ELEMENT_HPP
