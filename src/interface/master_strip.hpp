#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "audio_module.hpp"
#include "vu_meter.hpp"
#include "meter.hpp"
#include "../engine/master_node.hpp"

namespace Beam {

class MasterStrip : public Component {
public:
    MasterStrip(std::shared_ptr<MasterNode> node) : m_node(node) {
        m_vuMeter = std::make_shared<VUMeter>();
        m_levelMeterL = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Vertical);
        m_levelMeterR = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Vertical);
    }

    void setBounds(float x, float y, float w, float h) override {
        Component::setBounds(x, y, w, h);
        m_vuMeter->setBounds(x + 10, y + 40, w - 20, 80);
        m_levelMeterL->setBounds(x + 20, y + 130, 15, h - 180);
        m_levelMeterR->setBounds(x + w - 35, y + 130, 15, h - 180);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Main Background
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 10.0f, 1.0f, 0.15f, 0.16f, 0.18f, 1.0f);
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, 30, 10.0f, 0.5f, 0.22f, 0.24f, 0.28f, 1.0f);
        batcher.drawText("MASTER", m_bounds.x + 10, m_bounds.y + 8, 14, 1.0f, 0.8f, 0.2f, 1.0f);

        float peak = m_node ? m_node->getPeakLevel() : 0.0f;
        
        // Update Meters
        m_vuMeter->setLevel(peak);
        m_levelMeterL->setLevel(peak); 
        m_levelMeterR->setLevel(peak);

        m_vuMeter->render(batcher, dt, screenW, screenH);
        m_levelMeterL->render(batcher, dt, screenW, screenH);
        m_levelMeterR->render(batcher, dt, screenW, screenH);

        // Fader Background
        float faderX = m_bounds.x + m_bounds.w*0.5f - 10;
        float faderY = m_bounds.y + 130;
        float faderH = m_bounds.h - 180;
        batcher.drawQuad(faderX + 8, faderY, 4, faderH, 0.05f, 0.05f, 0.05f, 1.0f);

        // dB Ticks
        for (int db = 6; db >= -60; db -= 6) {
            float norm = (db + 60.0f) / 66.0f;
            float ty = faderY + (1.0f - norm) * faderH;
            batcher.drawQuad(faderX + 5, ty, 10, 1, 0.4f, 0.4f, 0.4f, 0.8f);
            if (db % 12 == 0) batcher.drawText(std::to_string(db), faderX - 15, ty - 4, 8, 0.5f, 0.5f, 0.5f, 1.0f);
        }

        // Fader Handle
        auto param = m_node->getParameter("Master Gain");
        float gVal = param ? param->getNormalizedValue() : 0.66f;
        float hy = faderY + (1.0f - gVal) * (faderH - 20);
        batcher.drawRoundedRect(faderX, hy, 20, 20, 4.0f, 0.5f, 0.8f, 0.8f, 0.9f, 1.0f);
        batcher.drawQuad(faderX + 2, hy + 9, 16, 2, 0.2f, 0.2f, 0.2f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (m_bounds.contains(x, y)) {
            m_isDraggingFader = true;
            return true;
        }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isDraggingFader && m_node) {
            float faderY = m_bounds.y + 130;
            float faderH = m_bounds.h - 180;
            float norm = 1.0f - (y - faderY) / faderH;
            m_node->getParameter("Master Gain")->setNormalizedValue(std::clamp(norm, 0.0f, 1.0f));
            return true;
        }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        m_isDraggingFader = false;
        return true;
    }

private:
    std::shared_ptr<MasterNode> m_node;
    std::shared_ptr<VUMeter> m_vuMeter;
    std::shared_ptr<LuminousMeter> m_levelMeterL;
    std::shared_ptr<LuminousMeter> m_levelMeterR;
    bool m_isDraggingFader = false;
};

} // namespace Beam

#endif