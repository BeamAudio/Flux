#ifndef ANALOG_UI_TEMPLATES_HPP
#define ANALOG_UI_TEMPLATES_HPP

#include "interface/core/component.hpp"
#include "interface/core/auto_flex_container.hpp"
#include "engine/plugins/flux_plugin.hpp"
#include "interface/core/ui_toolkit.hpp"
#include "interface/core/theme.hpp"
#include <vector>
#include <string>
#include <algorithm>

namespace Beam {

struct RackStyle {
    Color chassisColor;
    Color textColor;
    Theme::MaterialType material = Theme::MaterialType::Standard;
    Theme::KnobStyle knobStyle = Theme::KnobStyle::ClassicBakelite;
    bool showScrews = true;
    bool showMeter = false; // Generic GR meter
    bool invertMeter = false; // If true, grows from bottom (for input/level)
    std::string title;
    std::string subtitle;
};

/**
 * @class RackUnitUI
 * @brief A plugin UI using AutoFlexContainer for reliable knob layout.
 */
class RackUnitUI : public Component {
public:
    static constexpr float HEADER_H = 40.0f;
    static constexpr float PADDING = 10.0f; // Slightly more padding
    
    RackUnitUI(FluxNode* node, RackStyle style) : m_node(node), m_style(style) {
        setClipsChildren(true);
        
        // Create flex container for knobs
        AutoFlexContainer::Config cfg;
        cfg.direction = AutoFlexContainer::Direction::Row;
        cfg.crossAlign = AutoFlexContainer::Alignment::Start;
        cfg.gap = 20.0f; // More horizontal breathing room
        cfg.padding = 8.0f;
        cfg.preferredWidth = 400.0f; 
        cfg.wrap = true;
        
        m_flexContainer = std::make_shared<AutoFlexContainer>(cfg);
        addChildComponent(m_flexContainer);
        
        if (node) {
            setupControls();
        }
    }

    void setupControls() {
        auto params = m_node->getParameterOrder();
        for (const auto& param : params) {
            auto name = param->getName();
            auto knob = std::make_shared<Knob>(name, param->getMin(), param->getMax(), param->getValue());
            knob->bindParameter(param);
            knob->setStyle(m_style.knobStyle); // Apply style to all knobs in the rack
            knob->setName(name);
            knob->setClipsChildren(false);
            m_flexContainer->addFlexChild(knob);
            m_knobs.push_back(knob);
        }
    }

    void getPreferredSize(float& w, float& h) const override {
        float flexW = 0, flexH = 0;
        m_flexContainer->getPreferredSize(flexW, flexH);
        
        w = flexW + PADDING * 2;
        h = HEADER_H + flexH + PADDING * 2;
        
        if (w < 200.0f) w = 200.0f;
        if (m_style.showMeter) w += 35.0f;
    }

    void resized() override {
        float flexX = PADDING;
        float flexY = HEADER_H + PADDING;
        float flexW = m_bounds.w - PADDING * 2;
        float flexH = m_bounds.h - HEADER_H - PADDING * 2;
        
        if (m_style.showMeter) flexW -= 35.0f;
        
        m_flexContainer->setBounds(flexX, flexY, flexW, flexH);
        m_flexContainer->resized(); // This will trigger layout with hitboxes
    }

    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;
        Color c = m_style.chassisColor;

        // === ENHANCED PHYSICAL CHASSIS ===
        
        // 1. Drop Shadow
        batcher.drawRoundedRect(4, 5, w, h, 6.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.3f);
        
        // 2. Main Chassis with Material Texture
        batcher.drawChassisPanel(0, 0, w, h, 6.0f, c.r, c.g, c.b, 1.0f);
        
        // 3. Inner Panel Inset
        float inset = 8.0f;
        batcher.drawRoundedRect(inset, HEADER_H, w - inset*2, h - HEADER_H - inset, 5.0f, 0.5f, 
                                c.darker(0.15f).r, c.darker(0.15f).g, c.darker(0.15f).b, 1.0f);
        
        // 4. Screws (Aged Metal)
        if (m_style.showScrews) {
            auto drawScrew = [&](float sx, float sy) {
                batcher.drawRoundedRect(sx - 4 + 1, sy - 4 + 1, 8, 8, 4.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.4f);
                batcher.drawRoundedRect(sx - 4, sy - 4, 8, 8, 4.0f, 0.5f, 0.45f, 0.47f, 0.50f, 1.0f);
                batcher.drawLine(sx - 2, sy, sx + 2, sy, 1.5f, 0.1f, 0.1f, 0.1f, 0.8f);
            };
            drawScrew(10, 10); drawScrew(w - 10, 10); drawScrew(10, h - 10); drawScrew(w - 10, h - 10);
        }

        // 5. Title and Subtitle with Engraved effect
        Color tc = m_style.textColor;
        batcher.drawText(m_style.title, 24, 10, 15.0f, tc.darker(0.2f).r, tc.darker(0.2f).g, tc.darker(0.2f).b, 0.4f); // Emboss
        batcher.drawText(m_style.title, 23, 9, 15.0f, tc.r, tc.g, tc.b, 1.0f);
        
        if (!m_style.subtitle.empty()) {
            batcher.drawText(m_style.subtitle, 23, 26, 10.0f, tc.withAlpha(0.6f).r, tc.withAlpha(0.6f).g, tc.withAlpha(0.6f).b, 0.8f);
        }

        // 6. Meter Area
        if (m_style.showMeter && m_node->getMeterSource()->getNumMeters() > 0) {
            // ... (keep meter logic) ...
            float val = m_node->getMeterSource()->getValue(0);
            float meterX = w - 24.0f;
            float meterY = HEADER_H + 10.0f;
            float meterW = 12.0f;
            float meterH = h - HEADER_H - 20.0f;
            
            // Recessed Luminous Meter Slot
            batcher.drawRoundedRect(meterX - 2, meterY - 2, meterW + 4, meterH + 4, 3.0f, 0.5f, 0.05f, 0.05f, 0.06f, 1.0f);
            
            int numSegs = 14; 
            float gap = 2.0f;
            float segH = (meterH - (numSegs-1)*gap) / numSegs;
            for (int i = 0; i < numSegs; ++i) {
                float segPos = (m_style.invertMeter) ? (float)(numSegs - 1 - i) / (float)(numSegs - 1) : (float)i / (float)(numSegs - 1);
                
                // Light up based on value
                bool lit = (val > 0.005f) && (segPos < val); 
                
                Color col;
                if (m_style.invertMeter) {
                    col = (segPos < 0.7f) ? Theme::LEDGreen : (segPos < 0.9f ? Theme::LEDYellow : Theme::LEDRed);
                } else {
                    col = (segPos < 0.3f) ? Theme::LEDGreen : (segPos < 0.6f ? Theme::LEDYellow : Theme::LEDRed);
                }
                
                float alpha = lit ? 1.0f : 0.12f;
                batcher.drawRoundedRect(meterX, meterY + i*(segH + gap), meterW, segH, 1.5f, 0.5f, col.r, col.g, col.b, alpha);
                if (lit) {
                     batcher.drawRoundedRect(meterX - 1, meterY + i*(segH + gap) - 1, meterW + 2, segH + 2, 1.5f, 1.0f, col.r, col.g, col.b, 0.2f);
                }
            }
        }
        
        // 7. Draw Ticks around Knobs (High Detail)
        for (auto& knob : m_knobs) {
             float kx = knob->getX() + m_flexContainer->getX(); // Relative to RackUnit
             float ky = knob->getY() + m_flexContainer->getY();
             float kw = knob->getWidth();
             float kh = 50.0f; // Knob Circle Height
             float cx = kx + kw/2.0f;
             float cy = ky + kh/2.0f;
             float r = kh/2.0f + 4.0f; // Radius + gap
             
             int numTicks = 11;
             float startAngle = -2.356f; // -135 deg
             float endAngle = 2.356f;   // +135 deg
             
             for (int i=0; i<numTicks; ++i) {
                  float t = (float)i / (float)(numTicks - 1);
                  float a = startAngle + t * (endAngle - startAngle);
                  float tx1 = cx + std::sin(a) * r;
                  float ty1 = cy - std::cos(a) * r;
                  float tx2 = cx + std::sin(a) * (r + 3.0f);
                  float ty2 = cy - std::cos(a) * (r + 3.0f);
                  
                  batcher.drawLine(tx1, ty1, tx2, ty2, 1.0f, 0.3f, 0.3f, 0.3f, 0.8f);
             }
        }
    }

    // Predefined Styles with granular design tokens
    static RackStyle Pultec() { 
        return { {0.1f, 0.2f, 0.4f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, Theme::MaterialType::WrinklePaint, Theme::KnobStyle::ClassicBakelite, true, false, false, "TUBE-P", "PROGRAM EQUALIZER" }; 
    } 
    static RackStyle SSL() { 
        return { {0.3f, 0.3f, 0.3f, 1.0f}, {0.1f, 0.1f, 0.1f, 1.0f}, Theme::MaterialType::Standard, Theme::KnobStyle::ModernColored, true, false, false, "CONSOLE-E", "CHANNEL STRIP" }; 
    } 
    static RackStyle API() { 
        return { {0.1f, 0.1f, 0.1f, 1.0f}, {0.3f, 0.6f, 0.9f, 1.0f}, Theme::MaterialType::WrinklePaint, Theme::KnobStyle::ModernColored, true, false, false, "GRAPHIC-10", "DQS SYSTEM" }; 
    } 
    static RackStyle FET() { 
        return { {0.05f, 0.05f, 0.05f, 1.0f}, {0.9f, 0.9f, 0.9f, 1.0f}, Theme::MaterialType::Standard, Theme::KnobStyle::FlutedIndustrial, true, true, false, "FET-76", "LIMITING AMPLIFIER" }; 
    } 
    static RackStyle Aluminum() {
        return { {0.7f, 0.72f, 0.75f, 1.0f}, {0.1f, 0.1f, 0.12f, 1.0f}, Theme::MaterialType::BrushedAluminum, Theme::KnobStyle::BrushedAluminum, true, false, false, "MODERN", "ALUMINUM SERIES" };
    }
    static RackStyle Reverb(std::string name) { 
        return { {0.6f, 0.55f, 0.5f, 1.0f}, {0.2f, 0.1f, 0.0f, 1.0f}, Theme::MaterialType::Standard, Theme::KnobStyle::BrushedAluminum, true, false, false, name, "DIGITAL REVERB" }; 
    } 
    static RackStyle Delay(std::string name) { 
        return { {0.1f, 0.3f, 0.2f, 1.0f}, {0.8f, 1.0f, 0.8f, 1.0f}, Theme::MaterialType::Standard, Theme::KnobStyle::ClassicBakelite, true, false, false, name, "TAPE ECHO" }; 
    } 
    static RackStyle Utility(std::string name) { 
        return { {0.8f, 0.8f, 0.8f, 1.0f}, {0.15f, 0.15f, 0.18f, 1.0f}, Theme::MaterialType::Standard, Theme::KnobStyle::ModernColored, false, false, false, name, "UTILITY" }; 
    }
    static RackStyle Script(std::string name) { 
        return { {0.15f, 0.15f, 0.18f, 1.0f}, {0.2f, 0.9f, 1.0f, 1.0f}, Theme::MaterialType::Standard, Theme::KnobStyle::FlutedIndustrial, true, true, false, name, "FLUX SCRIPT ENGINE" }; 
    }

private:
    FluxNode* m_node;
    RackStyle m_style;
    std::vector<std::shared_ptr<Knob>> m_knobs;
    std::shared_ptr<AutoFlexContainer> m_flexContainer;
};

} // namespace Beam

#endif
