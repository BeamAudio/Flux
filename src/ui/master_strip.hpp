#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "component.hpp"
#include "../dsp/master_node.hpp"

namespace Beam {

class MasterStrip : public Component {
public:
    MasterStrip(std::shared_ptr<MasterNode> masterNode) : m_masterNode(masterNode) {}

    void render(QuadBatcher& batcher) override {
        // Background
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.12f, 0.12f, 0.13f, 1.0f);
        
        // VU Meter area
        float meterX = m_bounds.x + 10;
        float meterY = m_bounds.y + 20;
        float meterW = m_bounds.w - 20;
        float meterH = 150;
        batcher.drawQuad(meterX, meterY, meterW, meterH, 0.05f, 0.05f, 0.05f, 1.0f);
        
        if (m_masterNode) {
            float peak = m_masterNode->getPeakLevel();
            float levelH = peak * meterH;
            if (levelH > meterH) levelH = meterH;
            
            // Draw meter level (Green to Red gradient-ish)
            float r = (peak > 0.8f) ? 1.0f : 0.2f;
            float g = (peak > 0.8f) ? 0.2f : 0.8f;
            batcher.drawQuad(meterX + 2, meterY + meterH - levelH, meterW - 4, levelH, r, g, 0.2f, 1.0f);
        }
        
        // Master Fader track
        batcher.drawQuad(m_bounds.x + m_bounds.w/2 - 2, m_bounds.y + 200, 4, m_bounds.h - 250, 0.0f, 0.0f, 0.0f, 1.0f);
        
        // Master Fader Cap (Red for Master)
        batcher.drawQuad(m_bounds.x + 15, m_bounds.y + 400, m_bounds.w - 30, 15, 0.8f, 0.2f, 0.2f, 1.0f);
    }

private:
    std::shared_ptr<MasterNode> m_masterNode;
};

} // namespace Beam

#endif // MASTER_STRIP_HPP
