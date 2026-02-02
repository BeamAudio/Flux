#ifndef OPTO2A_UI_HPP
#define OPTO2A_UI_HPP

#include "interface/core/component.hpp"
#include "engine/plugins/flux_plugin.hpp"
#include "engine/nodes/analog_suite.hpp"
#include "interface/core/ui_toolkit.hpp"
#include "interface/core/theme.hpp"
#include <cmath>
#include <iomanip>

namespace Beam {

class Opto2A_UI : public Component {
public:
    Opto2A_UI(FluxPlugin* node) : m_node(node) {
        // Layout: 2U Rack size implication, but scaling to fit module window
        
        // Peak Reduction Knob
        auto pRedux = node->getParameter("Peak Redux");
        m_reductionKnob = std::make_shared<Knob>(pRedux->getName(), pRedux->getMin(), pRedux->getMax(), pRedux->getValue());
        m_reductionKnob->bindParameter(pRedux);
        m_reductionKnob->setName("Peak Reduction");
        addChildComponent(m_reductionKnob);

        // Gain Knob
        auto pGain = node->getParameter("Gain");
        m_gainKnob = std::make_shared<Knob>(pGain->getName(), pGain->getMin(), pGain->getMax(), pGain->getValue());
        m_gainKnob->bindParameter(pGain);
        m_gainKnob->setName("Make-Up Gain");
        addChildComponent(m_gainKnob);

        // Meter Source (GR)
        m_meterValue = 0.0f;
    }

    void resized() override {
        float w = m_bounds.w;
        float h = m_bounds.h;
        float knobSize = (std::min)(w * 0.25f, h * 0.6f);
        float yPos = h * 0.3f;

        m_reductionKnob->setBounds(w * 0.1f, yPos, knobSize, knobSize);
        m_gainKnob->setBounds(w * 0.65f, yPos, knobSize, knobSize);
    }

    void getPreferredSize(float& w, float& h) const override {
        w = 160.0f;
        h = 130.0f; 
    }

    void update(float dt) override {
        // Smooth needle movement
        if (auto* opto = dynamic_cast<Opto2A*>(m_node)) {
            float targetGR = opto->getLatestGR();
            // Convert linear GR (0..1 where 1 is no reduction) to dB roughly for visual
            // But GR return 1.0 - reduction. So 1.0 = 0dB GR, 0.5 = -6dB?
            // Actually implementation: maxGR = 1.0f - gr; so maxGR is positive magnitude 
            // of reduction. if gr=0.5, maxGR=0.5. 
            // Visual mapping: 0 -> 0dB, 1 -> -20dB?
            
            float db = (targetGR > 0.001f) ? 20.0f * std::log10(1.0f - targetGR) : 0.0f; 
            // Wait, targetGR is (1 - gr). If gr=1 (no redux), targetGR=0. 
            // If gr=0.5, targetGR=0.5. dB = 20log(0.5) = -6dB.
            // But the meter shows POSITIVE GR usually or Negative?
            // LA-2A meter shows Gain Reduction. Needle swings LEFT.
             
            m_meterValue = m_meterValue * 0.9f + targetGR * 0.1f;
        }
    }

    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;

        // 1. Faceplate (LA-2A Grey)
        // Brushed metal grey.
        batcher.drawChassisPanel(0, 0, w, h, 0.0f, 0.75f, 0.75f, 0.78f, 1.0f); 

        // 2. Upper Panel Section (Darker inset)
        batcher.drawRoundedRect(10, 10, w - 20, h * 0.35f, 4.0f, 0.5f, 0.2f, 0.2f, 0.2f, 0.3f);

        // 3. VU Meter Window
        float meterSize = w * 0.3f;
        float meterX = w * 0.35f;
        float meterY = h * 0.15f;
        
        // Meter Glass
        batcher.drawRoundedRect(meterX, meterY, meterSize, meterSize * 0.6f, 2.0f, 0.5f, 0.95f, 0.9f, 0.8f, 1.0f); // Warm white backlight
        
        // Needle
        // Angle 0 is up. -45 to +45 range.
        // GR 0dB = Right (+45 deg). Max GR = Left (-45 deg).
        // m_meterValue is 0.0 (no GR) to 1.0 (Full squash).
        // We want 0 -> +45 deg (Right), 1 -> -45 deg (Left).
        float angle = 0.785f - (m_meterValue * 1.57f * 2.0f); 
        // Clamp
        if (angle < -0.785f) angle = -0.785f;
        
        float cx = meterX + meterSize * 0.5f;
        float cy = meterY + meterSize * 0.8f; // Pivot low
        float r = meterSize * 0.5f;
        
        float nx = cx + std::sin(angle) * r;
        float ny = cy - std::cos(angle) * r;
        
        batcher.drawLine(cx, cy, nx, ny, 2.0f, 0.0f, 0.0f, 0.0f, 0.8f);

        // 4. Branding
        batcher.drawText("TELETRONIX", w * 0.1f, 10, 12.0f, 1.0f, 1.0f, 1.0f, 0.9f);
        batcher.drawText("MODEL LA-2A", w * 0.1f, 24, 10.0f, 0.8f, 0.0f, 0.0f, 0.9f);

        // 5. Screws
        auto screw = [&](float sx, float sy) {
             batcher.drawRoundedRect(sx-2, sy-2, 4.0f, 4.0f, 2.0f, 0.5f, 0.8f, 0.8f, 0.8f, 1.0f); // Head
             batcher.drawLine(sx-2, sy, sx+2, sy, 1.0f, 0.2f, 0.2f, 0.2f, 1.0f); // Slot
        };
        screw(10, h/2);
        screw(w-10, h/2);
        screw(10, h-10);
        screw(w-10, h-10);
    }

private:
    FluxPlugin* m_node;
    std::shared_ptr<Knob> m_reductionKnob;
    std::shared_ptr<Knob> m_gainKnob;
    float m_meterValue = 0.0f;
};

} // namespace Beam

#endif
