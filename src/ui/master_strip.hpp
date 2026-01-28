#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "component.hpp"
#include "vu_meter.hpp"
#include "../dsp/master_node.hpp"

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

    void render(QuadBatcher& batcher) override {
        // Console Metal Background
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 5.0f, 1.0f, 0.15f, 0.15f, 0.16f, 1.0f);
        
        if (m_masterNode) {
            m_vuMeter->setLevel(m_masterNode->getPeakLevel());
        }
        m_vuMeter->render(batcher);
        
        // Master Fader track (Etched look)
        float trackX = m_bounds.x + m_bounds.w * 0.5f - 2;
        float trackY = m_bounds.y + 120;
        float trackH = m_bounds.h - 160;
        batcher.drawRoundedRect(trackX, trackY, 4, trackH, 2.0f, 0.5f, 0.05f, 0.05f, 0.05f, 1.0f);
        
        // Master Fader Cap (Red plastic look)
        batcher.drawRoundedRect(m_bounds.x + 15, m_bounds.y + 350, m_bounds.w - 30, 25, 4.0f, 1.0f, 0.8f, 0.15f, 0.15f, 1.0f);
        batcher.drawQuad(m_bounds.x + 15, m_bounds.y + 361, m_bounds.w - 30, 3, 1.0f, 1.0f, 1.0f, 0.4f); // White line on cap

        batcher.drawText("MASTER", m_bounds.x + 20, m_bounds.y + m_bounds.h - 30, 12, 0.6f, 0.6f, 0.6f, 1.0f);
    }

private:
    std::shared_ptr<MasterNode> m_masterNode;
    std::shared_ptr<VUMeter> m_vuMeter;
};

} // namespace Beam

#endif // MASTER_STRIP_HPP