#ifndef AUDIO_CONFIG_VIEW_HPP
#define AUDIO_CONFIG_VIEW_HPP

#include "component.hpp"
#include "combo_box.hpp"
#include "../engine/audio_device_manager.hpp"
#include "../engine/audio_engine.hpp"
#include <vector>
#include <string>
#include <cmath>

namespace Beam {

class AudioConfigView : public Component {
public:
    AudioConfigView(AudioDeviceManager* manager, AudioEngine* engine) 
        : m_manager(manager), m_engine(engine) {
        setName("AudioConfigView");
        setVisible(false);
        
        m_outputCombo = std::make_shared<ComboBox>();
        m_inputCombo = std::make_shared<ComboBox>();
        m_sampleRateCombo = std::make_shared<ComboBox>();
        m_bufferSizeCombo = std::make_shared<ComboBox>();
        
        addChildComponent(m_outputCombo);
        addChildComponent(m_inputCombo);
        addChildComponent(m_sampleRateCombo);
        addChildComponent(m_bufferSizeCombo);

        setupCombos();
    }

    void setupCombos() {
        m_sampleRateCombo->addItem("44100");
        m_sampleRateCombo->addItem("48000");
        m_sampleRateCombo->addItem("88200");
        m_sampleRateCombo->addItem("96000");
        
        m_bufferSizeCombo->addItem("128");
        m_bufferSizeCombo->addItem("256");
        m_bufferSizeCombo->addItem("512");
        m_bufferSizeCombo->addItem("1024");
        
        m_outputCombo->setOnChange([this](int idx) { applyConfig(); });
        m_inputCombo->setOnChange([this](int idx) { applyConfig(); });
        m_sampleRateCombo->setOnChange([this](int idx) { applyConfig(); });
        m_bufferSizeCombo->setOnChange([this](int idx) { applyConfig(); });
    }

    void refreshDevices() {
        if (!m_manager) return;
        m_outputDevices = m_manager->getAvailableOutputDevices();
        m_inputDevices = m_manager->getAvailableInputDevices();
        m_currentSetup = m_manager->getCurrentDeviceSetup();
        
        m_outputCombo->clear();
        int outIdx = 0;
        for (int i=0; i<(int)m_outputDevices.size(); ++i) {
            m_outputCombo->addItem(m_outputDevices[i].name);
            if (m_outputDevices[i].deviceId == m_currentSetup.outputDeviceId) outIdx = i;
        }
        m_outputCombo->setSelectedId(outIdx);

        m_inputCombo->clear();
        int inIdx = 0;
        for (int i=0; i<(int)m_inputDevices.size(); ++i) {
            m_inputCombo->addItem(m_inputDevices[i].name);
            if (m_inputDevices[i].deviceId == m_currentSetup.inputDeviceId) inIdx = i;
        }
        m_inputCombo->setSelectedId(inIdx);
        
        if (m_currentSetup.sampleRate == 44100) m_sampleRateCombo->setSelectedId(0);
        else if (m_currentSetup.sampleRate == 48000) m_sampleRateCombo->setSelectedId(1);
        else if (m_currentSetup.sampleRate == 88200) m_sampleRateCombo->setSelectedId(2);
        else if (m_currentSetup.sampleRate == 96000) m_sampleRateCombo->setSelectedId(3);
        
        if (m_currentSetup.blockSize == 128) m_bufferSizeCombo->setSelectedId(0);
        else if (m_currentSetup.blockSize == 256) m_bufferSizeCombo->setSelectedId(1);
        else if (m_currentSetup.blockSize == 512) m_bufferSizeCombo->setSelectedId(2);
        else if (m_currentSetup.blockSize == 1024) m_bufferSizeCombo->setSelectedId(3);
    }

    void applyConfig() {
        if (!m_manager) return;
        int outIdx = m_outputCombo->getSelectedId();
        int inIdx = m_inputCombo->getSelectedId();
        if (outIdx < 0 || inIdx < 0) return;
        
        double sr = 44100;
        int srIdx = m_sampleRateCombo->getSelectedId();
        if (srIdx == 1) sr = 48000;
        if (srIdx == 2) sr = 88200;
        if (srIdx == 3) sr = 96000;
        
        int buf = 512;
        int bufIdx = m_bufferSizeCombo->getSelectedId();
        if (bufIdx == 0) buf = 128;
        if (bufIdx == 1) buf = 256;
        if (bufIdx == 2) buf = 512;
        if (bufIdx == 3) buf = 1024;
        
        m_manager->setCurrentAudioDevice(
            m_outputDevices[outIdx].name, m_outputDevices[outIdx].deviceId,
            m_inputDevices[inIdx].name, m_inputDevices[inIdx].deviceId,
            sr, buf);
            
        if (onConfigChanged) onConfigChanged();
    }

    void paint(QuadBatcher& batcher) override {
        if (!m_isVisible) return;

        batcher.drawQuad(0, 0, 5000, 5000, 0.0f, 0.0f, 0.0f, 0.7f);
        
        float winW = 600;
        float winH = 400;
        float winX = m_bounds.x + (m_bounds.w - winW) * 0.5f;
        float winY = m_bounds.y + (m_bounds.h - winH) * 0.5f;

        batcher.drawRoundedRect(winX, winY, winW, winH, 12.0f, 1.0f, 0.05f, 0.05f, 0.06f, 1.0f);
        batcher.drawRoundedRect(winX, winY, winW, 35, 12.0f, 0.5f, 0.13f, 0.62f, 0.42f, 1.0f);
        batcher.drawText("AUDIO SETTINGS", winX + 20, winY + 10, 14, 1.0f, 1.0f, 1.0f, 1.0f);

        float xOff = winX + 30;
        float yOff = winY + 60;

        auto drawLabel = [&](const std::string& text, float y) {
            batcher.drawText(text, xOff, y, 12, 0.8f, 0.8f, 0.8f, 1.0f);
        };

        drawLabel("Output Device:", yOff);
        drawLabel("Input Device:", yOff + 60);
        drawLabel("Sample Rate:", yOff + 120);
        drawLabel("Buffer Size:", yOff + 180);

        float closeBtnX = winX + winW - 40;
        float closeBtnY = winY + 7;
        batcher.drawRoundedRect(closeBtnX, closeBtnY, 25, 20, 4.0f, 0.5f, 0.56f, 0.03f, 0.03f, 1.0f);
        batcher.drawText("X", closeBtnX + 8, closeBtnY + 4, 12, 1.0f, 1.0f, 1.0f, 1.0f);
        
        // Component::render calls children paint
    }

    void resized() override {
        float winW = 600;
        float winH = 400;
        float winX = m_bounds.x + (m_bounds.w - winW) * 0.5f;
        float winY = m_bounds.y + (m_bounds.h - winH) * 0.5f;
        float xOff = winX + 150;
        float yOff = winY + 55;
        
        m_outputCombo->setBounds(xOff, yOff, 400, 24);
        m_inputCombo->setBounds(xOff, yOff + 60, 400, 24);
        m_sampleRateCombo->setBounds(xOff, yOff + 120, 150, 24);
        m_bufferSizeCombo->setBounds(xOff, yOff + 180, 150, 24);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return;
        paint(batcher); // Draw BG
        // Render children (Combos)
        for (auto& child : m_children) child->render(batcher, dt, screenW, screenH);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible) return false;
        
        // Check children first (Combos)
        if (Component::onMouseDown(x, y, button)) return true;

        float winW = 600;
        float winH = 400;
        float winX = m_bounds.x + (m_bounds.w - winW) * 0.5f;
        float winY = m_bounds.y + (m_bounds.h - winH) * 0.5f;
        
        // Close Button
        if (x > winX + winW - 45 && x < winX + winW && y > winY && y < winY + 35) {
            setVisible(false);
            return true;
        }
        
        // Consume click if inside window to prevent pass-through
        if (x >= winX && x <= winX + winW && y >= winY && y <= winY + winH) return true;
        
        return false; 
    }

    void setVisible(bool visible) override { 
        Component::setVisible(visible);
        if (visible) refreshDevices();
    }

    std::function<void()> onConfigChanged;

private:
    AudioDeviceManager* m_manager;
    AudioEngine* m_engine;
    std::vector<AudioDeviceInfo> m_outputDevices;
    std::vector<AudioDeviceInfo> m_inputDevices;
    AudioDeviceSetup m_currentSetup;
    
    std::shared_ptr<ComboBox> m_outputCombo;
    std::shared_ptr<ComboBox> m_inputCombo;
    std::shared_ptr<ComboBox> m_sampleRateCombo;
    std::shared_ptr<ComboBox> m_bufferSizeCombo;
};

} // namespace Beam

#endif