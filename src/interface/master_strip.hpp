#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "component.hpp"
#include "vu_meter.hpp"
#include "../engine/master_node.hpp"

namespace Beam {

class MasterStrip : public Component {
public:
    MasterStrip(std::shared_ptr<MasterNode> masterNode) : m_masterNode(masterNode) {
        m_vuMeter = std::make_shared<VUMeter>();
    }

    void setBounds(float x, float y, float w, float h) override {
        Component::setBounds(x, y, w, h);
        m_vuMeter->setBounds(x + 10, y + 20, w - 20, 80);
    }

    bool onMouseDown(float x, float y, int button) override {
        // Fader Interaction
        float faderY = getFaderY();
        if (y >= faderY && y <= faderY + 25) {
            m_isDragging = true;
            m_lastMouseY = y;
            return true;
        }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isDragging) {
            float trackY = m_bounds.y + 120;
            float trackH = m_bounds.h - 160;
            
            float normalized = (y - trackY) / trackH;
            normalized = 1.0f - std::clamp(normalized, 0.0f, 1.0f);
            
            if (m_masterNode) {
                m_masterNode->getParameter("Master Gain")->setNormalizedValue(normalized);
            }
            return true;
        }
        return false;
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 5.0f, 1.0f, 0.15f, 0.15f, 0.16f, 1.0f);
        
        if (m_masterNode) {
            m_vuMeter->setLevel(m_masterNode->getPeakLevel());
        }
        m_vuMeter->render(batcher, dt, screenW, screenH);
        
        float trackX = m_bounds.x + m_bounds.w * 0.5f - 2;
        float trackY = m_bounds.y + 120;
        float trackH = m_bounds.h - 160;
        
        // Fader Ticks
        float tickVals[] = {1.0f, 0.707f, 0.5f, 0.25f, 0.0f}; // Linear approx
        const char* labels[] = {"+10", "0", "-10", "-20", "-inf"};
        for(int i=0; i<5; ++i) {
            float y = trackY + (1.0f - tickVals[i]) * trackH; // Fader maps 1.0 (top) to 0.0 (bottom) usually? 
            // Wait, getFaderY uses (1.0 - norm). So 1.0 is Top.
            // If Master Gain 1.0 = 0dB? Or +10dB? Usually Max is > 0dB.
            // Let's assume Max = +10dB.
            // 0 dB is usually around 0.7-0.8 normalized.
            // Let's stick to simple visual markers.
            
            float ty = trackY + (float)i / 4.0f * trackH;
            batcher.drawQuad(trackX - 8, ty, 4, 1, 0.4f, 0.4f, 0.4f, 1.0f);
            batcher.drawText(labels[4-i], trackX - 25, ty - 4, 9, 0.6f, 0.6f, 0.6f, 1.0f);
        }

        batcher.drawRoundedRect(trackX, trackY, 4, trackH, 2.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
        
        // Dynamic Fader Cap position
        float capY = getFaderY();
        batcher.drawRoundedRect(m_bounds.x + 15, capY, m_bounds.w - 30, 25, 4.0f, 1.0f, 0.8f, 0.15f, 0.15f, 1.0f);
        batcher.drawQuad(m_bounds.x + 15, capY + 11, m_bounds.w - 30, 3, 1.0f, 1.0f, 1.0f, 0.4f);

        batcher.drawText("MASTER", m_bounds.x + 20, m_bounds.y + m_bounds.h - 30, 12, 0.6f, 0.6f, 0.6f, 1.0f);
    }

private:
    float getFaderY() {
        float trackY = m_bounds.y + 120;
        float trackH = m_bounds.h - 160;
        float norm = 0.5f;
        if (m_masterNode) norm = m_masterNode->getParameter("Master Gain")->getNormalizedValue();
        return trackY + (1.0f - norm) * trackH - 12.5f;
    }

    std::shared_ptr<MasterNode> m_masterNode;
    std::shared_ptr<VUMeter> m_vuMeter;
};

} // namespace Beam

#endif // MASTER_STRIP_HPP






