#ifndef MASTER_STRIP_HPP
#define MASTER_STRIP_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/needle_meter.hpp"
#include "interface/widgets/meter.hpp"
#include "interface/widgets/slider.hpp"
#include "interface/core/layout.hpp"
#include "engine/core/audio_engine.hpp"
#include "engine/dsp/flux_audio_utils.hpp"

namespace Beam {

/**
 * @class MasterStrip
 * @brief High-fidelity master output strip with integrated meters and gain control.
 */
class MasterStrip : public Component {
public:
    MasterStrip(AudioEngine* engine) : m_engine(engine) {
        setName("MasterStrip");
        
        m_vuMeter = std::make_shared<NeedleMeter>();
        m_vuMeter->setName("MasterVU");

        m_levelMeterL = std::make_shared<LuminousMeter>();
        m_levelMeterL->setOrientation(LuminousMeter::Orientation::Vertical);
        m_levelMeterL->setName("PeakL");

        m_levelMeterR = std::make_shared<LuminousMeter>();
        m_levelMeterR->setOrientation(LuminousMeter::Orientation::Vertical);
        m_levelMeterR->setName("PeakR");
        
        m_gainSlider = std::make_shared<Slider>();
        m_gainSlider->setSliderStyle(SliderStyle::LinearVertical);
        // Custom Harrison Fader Cap color
        m_gainSlider->setClipsChildren(false);

        if (m_engine && m_engine->getMasterNode()) {
            m_gainSlider->setParameter(m_engine->getMasterNode()->getParameter("Master Gain"));
        }

        addChildComponent(m_vuMeter);
        addChildComponent(m_levelMeterL);
        addChildComponent(m_levelMeterR);
        addChildComponent(m_gainSlider);
    }

    void setBounds(float x, float y, float width, float height) override {
        Component::setBounds(x, y, width, height);

        // 1. VU Meter Position (Centered at Top)
        float vuH = 65.0f;
        m_vuMeter->setBounds(5, 45, width - 10, vuH);

        // 2. Fader and Peak Meters Area
        float faderY = 130.0f;
        float faderH = height - faderY - 20;
        
        float meterW = 12.0f;
        float sliderW = 32.0f;
        float spacing = 8.0f;
        
        float totalW = meterW * 2 + sliderW + spacing * 2;
        float startX = (width - totalW) * 0.5f;
        
        m_levelMeterL->setBounds(startX, faderY, meterW, faderH);
        m_gainSlider->setBounds(startX + meterW + spacing, faderY, sliderW, faderH);
        m_levelMeterR->setBounds(startX + meterW + sliderW + spacing * 2, faderY, meterW, faderH);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        float peak = (m_engine && m_engine->getMasterNode()) ? m_engine->getMasterNode()->getPeakLevel() : 0.0f;
        
        m_vuMeter->setValue(peak); // Updated for NeedleMeter
        m_levelMeterL->setLevel(peak); 
        m_levelMeterR->setLevel(peak);

        Component::render(batcher, dt, screenW, screenH);
    }

    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;

        // 1. Harrison Console Material (Wrinkle Paint / Deep Charcoal)
        batcher.drawChassisPanel(0, 0, w, h, 0.0f, 0.08f, 0.08f, 0.09f, 1.0f);
        
        // 2. Head Plate (Aluminum with brand etching)
        batcher.drawRoundedGradientRect(4, 4, w - 8, 32, 2.0f, 0.5f,
                                       0.75f, 0.77f, 0.80f, 1.0f, 
                                       0.50f, 0.52f, 0.55f, 1.0f);
        
        batcher.drawText("MIXBUS MASTER", 15, 10, 14.0f, 0.1f, 0.1f, 0.15f, 1.0f);
        batcher.drawText("ANALOG CONSOLE", 15, 26, 8.0f, 0.3f, 0.3f, 0.35f, 0.8f);

        // 3. Gold Mounting Screws
        auto drawBrassScrew = [&](float sx, float sy) {
            batcher.drawRoundedRect(sx - 3, sy - 3, 6, 6, 3.0f, 0.5f, 0.7f, 0.6f, 0.3f, 1.0f);
            batcher.drawLine(sx - 1.5f, sy, sx + 1.5f, sy, 1.5f, 0.2f, 0.15f, 0.05f, 1.0f);
        };
        drawBrassScrew(w - 12, 12); drawBrassScrew(w - 12, h - 12);
        drawBrassScrew(12, h - 12);

        // 4. Detailed Metering & Fader Scale
        float fx = w * 0.5f;
        float fy = 200.0f; // Start of fader scale
        float fh = h - fy - 40.0f;

        // Vintage Scale markings
        for (int db = 6; db >= -60; db -= 6) {
            float norm = (db + 60.0f) / 66.0f;
            float ty = fy + (1.0f - norm) * fh;
            float lineW = (db % 12 == 0) ? 15.0f : 8.0f;
            batcher.drawLine(fx - lineW, ty, fx + lineW, ty, 1.5f, 1.0f, 1.0f, 1.0f, (db == 0) ? 0.8f : 0.4f);
            
            if (db % 12 == 0 || db == 0) {
                batcher.drawText(std::to_string(db), fx + 18, ty - 5, 10.0f, 0.9f, 0.9f, 0.9f, 0.8f);
            }
        }
    }

private:
    AudioEngine* m_engine;
    std::shared_ptr<NeedleMeter> m_vuMeter;
    std::shared_ptr<LuminousMeter> m_levelMeterL;
    std::shared_ptr<LuminousMeter> m_levelMeterR;
    std::shared_ptr<Slider> m_gainSlider;
};

} // namespace Beam

#endif