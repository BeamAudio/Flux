#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "audio_module.hpp"
#include "vu_meter.hpp"
#include "meter.hpp"
#include "../engine/master_node.hpp"

namespace Beam {

/**
 * @class MasterStrip
 * @brief High-fidelity master output strip with integrated meters and gain control.
 */
class MasterStrip : public Component {
public:
    MasterStrip(std::shared_ptr<MasterNode> node) : m_node(node) {
        m_vuMeter = std::make_shared<VUMeter>();
        m_levelMeterL = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Vertical);
        m_levelMeterR = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Vertical);
    }

    void setBounds(float x, float y, float w, float h) override {
        Component::setBounds(x, y, w, h);
        m_vuMeter->setBounds(x + 5, y + 35, w - 10, 70);
        m_levelMeterL->setBounds(x + 10, y + 115, 12, h - 145);
        m_levelMeterR->setBounds(x + w - 22, y + 115, 12, h - 145);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Main Frame
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 10.0f, 1.0f, 0.12f, 0.12f, 0.14f, 1.0f);
        // Header
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, 30, 10.0f, 0.5f, 0.18f, 0.2f, 0.25f, 1.0f);
        batcher.drawText("MASTER", m_bounds.x + (m_bounds.w - 60)/2, m_bounds.y + 8, 12, 1.0f, 0.8f, 0.2f, 1.0f);

        float peak = m_node ? m_node->getPeakLevel() : 0.0f;
        
        m_vuMeter->setLevel(peak);
        m_levelMeterL->setLevel(peak); 
        m_levelMeterR->setLevel(peak);

        m_vuMeter->render(batcher, dt, screenW, screenH);
        m_levelMeterL->render(batcher, dt, screenW, screenH);
        m_levelMeterR->render(batcher, dt, screenW, screenH);

        // Fader Gutter
        float fx = m_bounds.x + (m_bounds.w - 6)/2;
        float fy = m_bounds.y + 115;
        float fh = m_bounds.h - 145;
        batcher.drawQuad(fx, fy, 6, fh, 0.02f, 0.02f, 0.02f, 1.0f);

        // dB Scale
        for (int db = 6; db >= -60; db -= 6) {
            float norm = (db + 60.0f) / 66.0f;
            float ty = fy + (1.0f - norm) * fh;
            batcher.drawQuad(fx - 4, ty, 14, 1, 0.3f, 0.3f, 0.3f, 0.5f);
            if (db % 12 == 0) {
                batcher.drawText(std::to_string(db), fx - 22, ty - 4, 8, 0.5f, 0.5f, 0.5f, 1.0f);
            }
        }

        // Fader Handle
        auto param = m_node->getParameter("Master Gain");
        float gVal = param ? param->getNormalizedValue() : 0.66f;
        float hy = fy + (1.0f - gVal) * (fh - 30);
        
        batcher.drawRoundedRect(fx - 12, hy, 30, 30, 4.0f, 0.5f, 0.7f, 0.7f, 0.75f, 1.0f);
        batcher.drawQuad(fx - 10, hy + 14, 26, 2, 0.1f, 0.1f, 0.1f, 1.0f); // Center line
    }

    bool onMouseDown(float x, float y, int button) override {
        float fx = m_bounds.x + (m_bounds.w - 30)/2;
        float fy = m_bounds.y + 115;
        float fh = m_bounds.h - 145;
        Rect gutter = {fx, fy, 30, fh};
        
        if (gutter.contains(x, y)) {
            m_isDraggingFader = true;
            updateFader(y);
            return true;
        }
        return false;
    }

    bool onMouseMove(float x, float y) override {
        if (m_isDraggingFader) {
            updateFader(y);
            return true;
        }
        return false;
    }

    bool onMouseUp(float x, float y, int button) override {
        m_isDraggingFader = false;
        return true;
    }

private:
    void updateFader(float y) {
        if (!m_node) return;
        float fy = m_bounds.y + 115;
        float fh = m_bounds.h - 145;
        float norm = 1.0f - (y - fy) / fh;
        m_node->getParameter("Master Gain")->setNormalizedValue(std::clamp(norm, 0.0f, 1.0f));
    }

    std::shared_ptr<MasterNode> m_node;
    std::shared_ptr<VUMeter> m_vuMeter;
    std::shared_ptr<LuminousMeter> m_levelMeterL;
    std::shared_ptr<LuminousMeter> m_levelMeterR;
    bool m_isDraggingFader = false;
};

} // namespace Beam

#endif
