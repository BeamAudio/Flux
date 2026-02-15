#ifndef CHANNEL_STRIP_HPP
#define CHANNEL_STRIP_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/needle_meter.hpp"
#include "interface/widgets/meter.hpp"
#include "interface/widgets/slider.hpp"
#include "interface/widgets/knob.hpp"
#include "interface/widgets/button.hpp"
#include "engine/session/mixer_state.hpp"
#include "engine/core/flux_node.hpp"
#include <string>

namespace Beam {

/**
 * @class ChannelStrip
 * @brief A vertical channel strip for the Mixer View with Harrison console styling.
 */
class ChannelStrip : public Component {
public:
    ChannelStrip(const std::string& name, MixChannel* mixChannel, size_t nodeId, std::shared_ptr<FluxNode> node = nullptr)
        : m_channelName(name), m_mixChannel(mixChannel), m_nodeId(nodeId), m_node(node) 
    {
        setName("ChannelStrip_" + name);
        
        // --- Panning ---
        m_panKnob = std::make_shared<Knob>("PAN", 0.0f, 1.0f, 0.5f);
        m_panKnob->setStyle(Theme::KnobStyle::ModernColored);
        // Note: We don't bind to node parameter here anymore, 
        // we sync manually in render() to the MixChannel state.
        addChildComponent(m_panKnob);

        // --- Sends ---
        m_send1Knob = std::make_shared<Knob>("S1", 0.0f, 1.0f, 0.0f);
        m_send1Knob->setStyle(Theme::KnobStyle::ModernColored);
        if (m_node) {
            if (auto p = m_node->getParameter("Send 1 Level")) m_send1Knob->bindParameter(p);
        }
        addChildComponent(m_send1Knob);

        m_send2Knob = std::make_shared<Knob>("S2", 0.0f, 1.0f, 0.0f);
        m_send2Knob->setStyle(Theme::KnobStyle::ModernColored);
        if (m_node) {
            if (auto p = m_node->getParameter("Send 2 Level")) m_send2Knob->bindParameter(p);
        }
        addChildComponent(m_send2Knob);

        // Luminous Peak Meter
        m_peakMeter = std::make_shared<LuminousMeter>();
        m_peakMeter->setOrientation(LuminousMeter::Orientation::Vertical);
        m_peakMeter->setName("Peak");
        addChildComponent(m_peakMeter);
        
        // Create a gain parameter (dB Scale: -60 to +6)
        m_gainParam = std::make_shared<Parameter>("Gain", -60.0f, 6.0f, 0.0f);
        
        // Fader - control gain via parameter
        m_fader = std::make_shared<Slider>(m_gainParam);
        m_fader->setSliderStyle(SliderStyle::LinearVertical);
        m_fader->setRange(-60.0f, 6.0f);
        m_fader->setDefaultResetValue(0.0); // Reset to 0dB (unity)
        m_fader->setName("Fader");
        addChildComponent(m_fader);
        
        // Mute Button
        m_muteButton = std::make_shared<Button>("M");
        m_muteButton->setName("Mute");
        m_muteButton->setClickingTogglesState(true);
        m_muteButton->onClick([this]() {
            if (m_mixChannel) {
                m_mixChannel->muted.store(m_muteButton->getToggleState(), std::memory_order_relaxed);
            }
        });
        addChildComponent(m_muteButton);
        
        // Solo Button
        m_soloButton = std::make_shared<Button>("S");
        m_soloButton->setName("Solo");
        m_soloButton->setClickingTogglesState(true);
        m_soloButton->onClick([this]() {
            if (m_mixChannel) {
                m_mixChannel->solo.store(m_soloButton->getToggleState(), std::memory_order_relaxed);
            }
        });
        addChildComponent(m_soloButton);
    }
    
    void setMeterLevel(float level) {
        m_peakMeter->setLevel(level);
    }
    
    void setBounds(float x, float y, float width, float height) override {
        Component::setBounds(x, y, width, height);
        
        float padding = 6.0f;
        float headerH = 20.0f;
        float panH = 55.0f;
        float sendsH = 50.0f;
        float buttonsH = 44.0f;
        float labelH = 22.0f;
        
        // 1. Pan Knob at Top
        m_panKnob->setBounds((width - 45.0f)/2.0f, headerH + padding, 45, 45);

        // 2. Sends side-by-side
        float sendW = (width - 3*padding) / 2.0f;
        m_send1Knob->setBounds(padding, headerH + padding + panH, sendW, sendsH);
        m_send2Knob->setBounds(padding*2 + sendW, headerH + padding + panH, sendW, sendsH);

        // 3. Meter & Fader
        float meterW = 12.0f;
        float faderW = 28.0f;
        float middleY = headerH + padding*2 + panH + sendsH;
        float bottomY = height - labelH - buttonsH - padding*2;
        float faderH = bottomY - middleY;

        float totalW = meterW + padding + faderW;
        float startX = (width - totalW) / 2.0f;
        
        m_peakMeter->setBounds(startX, middleY, meterW, faderH);
        m_fader->setBounds(startX + meterW + padding, middleY, faderW, faderH);
        
        // 4. Buttons
        float buttonW = (width - 3*padding) / 2.0f;
        float buttonsTop = height - labelH - buttonsH - padding;
        m_muteButton->setBounds(padding, buttonsTop, buttonW, 22);
        m_soloButton->setBounds(padding + buttonW + padding, buttonsTop, buttonW, 22);
    }
    
    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Bidirectional sync between Parameter/UI and atomic
        if (m_mixChannel) {
            if (m_gainParam) {
                // Write parameter value to atomic (user is dragging the fader)
                float db = m_gainParam->getValue();
                float linear = (db <= -60.0f) ? 0.0f : std::pow(10.0f, db / 20.0f);
                m_mixChannel->gain.store(linear, std::memory_order_relaxed);
            }

            if (m_panKnob) {
                m_mixChannel->pan.store(m_panKnob->getValue(), std::memory_order_relaxed);
            }
            
            // Update button visuals
            m_muteButton->setToggleState(m_mixChannel->muted.load(std::memory_order_relaxed), false);
            m_soloButton->setToggleState(m_mixChannel->solo.load(std::memory_order_relaxed), false);
        }
        Component::render(batcher, dt, screenW, screenH);
    }
    
    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;
        
        // Harrison Console Material
        batcher.drawChassisPanel(0, 0, w, h, 0.0f, 0.22f, 0.22f, 0.25f, 1.0f);
        
        // Name Plate at Bottom (Scribble Strip)
        float plateH = 20.0f;
        float plateY = h - plateH - 2.0f;
        batcher.drawRoundedGradientRect(3, plateY, w - 6, plateH, 2.0f, 0.5f,
                                       0.85f, 0.87f, 0.90f, 1.0f, 
                                       0.70f, 0.72f, 0.75f, 1.0f);
        
        // Channel name (Black text on light plate)
        batcher.drawVectorText(m_channelName, 6, plateY + 4, 10.0f, 0.05f, 0.05f, 0.05f, 1.0f);
        
        // Channel Number / ID at top
        std::string idStr = std::to_string(m_nodeId);
        batcher.drawVectorText(idStr, w/2 - 4, 6, 12.0f, 0.4f, 0.4f, 0.45f, 1.0f);
        
        if (!m_fader) return;
        
        // Muted indicator
        if (m_mixChannel && m_mixChannel->muted.load(std::memory_order_relaxed)) {
            batcher.drawRoundedRect(2, 2, w - 4, 14, 2.0f, 0.5f, 0.9f, 0.2f, 0.1f, 0.6f);
        }
        
        // dB Scale Logic
        float faderY = m_fader->getY();
        float faderH = m_fader->getHeight();
        float scaleX = m_fader->getX() + m_fader->getWidth() + 2.0f;
        
        if (faderH < 10.0f) return;

        for (int db = 6; db >= -60; db -= 6) {
             // Map dB -60..6 to 0..1
             float norm = (float)(db + 60) / 66.0f;
             float ty = faderY + faderH - (norm * faderH);
             
             if (ty >= faderY && ty <= faderY + faderH) {
                 float lineW = (db == 0) ? 6.0f : 3.0f;
                 // Draw Tick
                 if (db == 0) {
                     batcher.drawLine(scaleX, ty, scaleX + lineW, ty, 1.5f, 1.0f, 0.2f, 0.2f, 1.0f);
                     batcher.drawVectorText("0", scaleX + 8, ty - 4, 9, 1.0f, 0.2f, 0.2f, 1.0f);
                 } else {
                     batcher.drawLine(scaleX, ty, scaleX + lineW, ty, 1.0f, 0.7f, 0.75f, 0.8f, 0.6f);
                     if (db == 6 || db == -6 || db == -12 || db == -24 || db == -48) {
                         batcher.drawVectorText(std::to_string(db), scaleX + 6, ty - 3, 8, 0.5f, 0.5f, 0.55f, 1.0f);
                     }
                 }
             }
        }
    }

    size_t getNodeId() const { return m_nodeId; }

private:
    std::string m_channelName;
    MixChannel* m_mixChannel;
    size_t m_nodeId;
    std::shared_ptr<FluxNode> m_node;
    
    std::shared_ptr<Parameter> m_gainParam;
    std::shared_ptr<Knob> m_panKnob;
    std::shared_ptr<Knob> m_send1Knob;
    std::shared_ptr<Knob> m_send2Knob;
    std::shared_ptr<LuminousMeter> m_peakMeter;
    std::shared_ptr<Slider> m_fader;
    std::shared_ptr<Button> m_muteButton;
    std::shared_ptr<Button> m_soloButton;
};

} // namespace Beam

#endif // CHANNEL_STRIP_HPP
