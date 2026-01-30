#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "component.hpp"
#include "vu_meter.hpp"
#include "meter.hpp"
#include "slider.hpp"
#include "../engine/master_node.hpp"
#include "../utilities/flux_audio_utils.hpp"

namespace Beam {

/**
 * @class MasterStrip
 * @brief High-fidelity master output strip with integrated meters and gain control.
 */
class MasterStrip : public Component {
public:
    MasterStrip(std::shared_ptr<MasterNode> node) : m_node(node) {
        setName("MasterStrip");
        setDraggable(true);
        
        m_vuMeter = std::make_shared<VUMeter>();
        m_vuMeter->setInterceptsMouseClicks(false);

        m_levelMeterL = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Vertical);
        m_levelMeterL->setInterceptsMouseClicks(false);

        m_levelMeterR = std::make_shared<LuminousMeter>(LuminousMeter::Orientation::Vertical);
        m_levelMeterR->setInterceptsMouseClicks(false);
        
        m_gainSlider = std::make_shared<Slider>();
        m_gainSlider->setSliderStyle(SliderStyle::LinearVertical);
        if (m_node) {
            m_gainSlider->setParameter(m_node->getParameter("Master Gain"));
        }

        addChildComponent(m_vuMeter);
        addChildComponent(m_levelMeterL);
        addChildComponent(m_levelMeterR);
        addChildComponent(m_gainSlider);
    }

    void setBounds(float x, float y, float width, float height) override {
        Component::setBounds(x, y, width, height);
        m_vuMeter->setBounds(x + 5, y + 35, width - 10, 70);
        m_levelMeterL->setBounds(x + 10, y + 115, 12, height - 145);
        m_levelMeterR->setBounds(x + width - 22, y + 115, 12, height - 145);
        
        float fx = x + (width - 30)/2;
        m_gainSlider->setBounds(fx, y + 115, 30, height - 145);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        float peak = m_node ? m_node->getPeakLevel() : 0.0f;
        
        m_vuMeter->setLevel(peak);
        m_levelMeterL->setLevel(peak); 
        m_levelMeterR->setLevel(peak);

        Component::render(batcher, dt, screenW, screenH);
    }

    void paint(QuadBatcher& batcher) override {
        // Main Frame
        batcher.drawRoundedGradientRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 10.0f, 1.0f, 
                                       0.05f, 0.05f, 0.06f, 1.0f, // BRAND_BLACK
                                       0.1f, 0.1f, 0.12f, 1.0f);
        // Header
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, 30, 10.0f, 0.5f, 0.13f, 0.62f, 0.42f, 1.0f); // BRAND_EMERALD
        batcher.drawText("MASTER", m_bounds.x + (m_bounds.w - AudioUtils::calculateTextWidth("MASTER", 12))/2, m_bounds.y + 8, 12, 1.0f, 1.0f, 1.0f, 1.0f);

        // Fader Gutter area
        float fx = m_bounds.x + (m_bounds.w - 6)/2;
        float fy = m_bounds.y + 115;
        float fh = m_bounds.h - 145;

        // dB Scale
        for (int db = 6; db >= -60; db -= 6) {
            float norm = (db + 60.0f) / 66.0f;
            float ty = fy + (1.0f - norm) * fh;
            batcher.drawQuad(fx - 15, ty, 30, 1, 0.95f, 0.95f, 0.95f, 0.2f); // BRAND_WHITE
            if (db % 12 == 0) {
                batcher.drawText(std::to_string(db), fx - 25, ty - 4, 8, 0.95f, 0.95f, 0.95f, 0.6f);
            }
        }
    }

private:
    std::shared_ptr<MasterNode> m_node;
    std::shared_ptr<VUMeter> m_vuMeter;
    std::shared_ptr<LuminousMeter> m_levelMeterL;
    std::shared_ptr<LuminousMeter> m_levelMeterR;
    std::shared_ptr<Slider> m_gainSlider;
};

} // namespace Beam

#endif