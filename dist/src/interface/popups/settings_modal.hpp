#ifndef SETTINGS_MODAL_HPP
#define SETTINGS_MODAL_HPP

#include "interface/core/component.hpp"
#include "interface/widgets/button.hpp"
#include "interface/widgets/label.hpp"
#include "interface/widgets/slider.hpp"
#include "interface/widgets/combo_box.hpp"
#include "engine/session/beam_host.hpp"
#include "engine/core/audio_device_manager.hpp"

namespace Beam {

class SettingsModal : public Component, public PopupHost {
public:
    SettingsModal(BeamHost* host, AudioDeviceManager* deviceManager) 
        : m_host(host), m_deviceManager(deviceManager) {
        setName("SettingsModal");
        
        // --- Tabs ---
        m_generalTabBtn = std::make_shared<Button>("GENERAL");
        m_generalTabBtn->setClickingTogglesState(true);
        m_generalTabBtn->setToggleState(true);
        m_generalTabBtn->onClick([this]() { setTab(0); });
        
        m_audioTabBtn = std::make_shared<Button>("AUDIO");
        m_audioTabBtn->setClickingTogglesState(true);
        m_audioTabBtn->setToggleState(false);
        m_audioTabBtn->onClick([this]() { setTab(1); });

        addChildComponent(m_generalTabBtn);
        addChildComponent(m_audioTabBtn);

        // --- General Controls ---
        m_autosaveToggle = std::make_shared<Button>("ON");
        m_autosaveToggle->setClickingTogglesState(true);
        m_intervalSlider = std::make_shared<Slider>();
        m_intervalSlider->setRange(1.0f, 60.0f);
        
        addChildComponent(m_autosaveToggle);
        addChildComponent(m_intervalSlider);

        // --- Audio Controls ---
        m_sampleRateCombo = std::make_shared<ComboBox>();
        m_bufferSizeCombo = std::make_shared<ComboBox>();
        m_scanPluginsBtn = std::make_shared<Button>("RE-SCAN PLUGINS");
        
        m_sampleRateCombo->addItem("44100");
        m_sampleRateCombo->addItem("48000");
        m_sampleRateCombo->addItem("88200");
        m_sampleRateCombo->addItem("96000");
        
        m_bufferSizeCombo->addItem("128");
        m_bufferSizeCombo->addItem("256");
        m_bufferSizeCombo->addItem("512");
        m_bufferSizeCombo->addItem("1024");
        
        m_sampleRateCombo->setOnChange([this](int) { applyAudioSettings(); });
        m_bufferSizeCombo->setOnChange([this](int) { applyAudioSettings(); });
        m_scanPluginsBtn->onClick([this]() {
             // Trigger re-scan
             // We can call into BeamHost or straight to compiler if included
        });

        addChildComponent(m_sampleRateCombo);
        addChildComponent(m_bufferSizeCombo);
        addChildComponent(m_scanPluginsBtn);

        // --- Close ---
        m_closeBtn = std::make_shared<Button>("Close");
        m_closeBtn->onClick([this]() {
            m_host->saveSettings();
            if (onClose) onClose();
        });
        addChildComponent(m_closeBtn);

        loadValues();
        setTab(0);
    }

    void loadValues() {
        m_isLoading = true;
        // General
        auto& settings = m_host->getSettings();
        m_autosaveToggle->setToggleState(settings.autosaveEnabled);
        m_autosaveToggle->setButtonText(settings.autosaveEnabled ? "ON" : "OFF");
        
        m_intervalSlider->setValue(settings.autosaveIntervalMinutes);
        
        // Audio
        if (m_deviceManager) {
            auto setup = m_deviceManager->getCurrentDeviceSetup();
            if (setup.sampleRate == 44100) m_sampleRateCombo->setSelectedId(0);
            else if (setup.sampleRate == 48000) m_sampleRateCombo->setSelectedId(1);
            else if (setup.sampleRate == 88200) m_sampleRateCombo->setSelectedId(2);
            else if (setup.sampleRate == 96000) m_sampleRateCombo->setSelectedId(3);
            
            if (setup.blockSize == 128) m_bufferSizeCombo->setSelectedId(0);
            else if (setup.blockSize == 256) m_bufferSizeCombo->setSelectedId(1);
            else if (setup.blockSize == 512) m_bufferSizeCombo->setSelectedId(2);
            else if (setup.blockSize == 1024) m_bufferSizeCombo->setSelectedId(3);
        }
        m_isLoading = false;
    }

    void applyAudioSettings() {
        if (m_isLoading) return;
        if (!m_deviceManager) return;
        
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
        
        auto current = m_deviceManager->getCurrentDeviceSetup();
        m_deviceManager->setCurrentAudioDevice(current.outputDeviceName, current.outputDeviceId,
                                               current.inputDeviceName, current.inputDeviceId,
                                               sr, buf);
                                               
        // Update AppSettings
        auto& settings = m_host->getSettings();
        settings.audio.sampleRate = sr;
        settings.audio.bufferSize = buf;
        settings.audio.outputDeviceId = current.outputDeviceId;
        settings.audio.inputDeviceId = current.inputDeviceId;
        
        // Save immediately when audio settings change
        m_host->saveSettings();
    }



    // PopupHost Implementation
    void showPopup(std::shared_ptr<Component> popup) override {
        m_popup = popup;
        if(m_popup) m_popup->setParent(this);
    }

    void closePopup() override {
        m_popup = nullptr;
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        if (m_popup) {
             if (m_popup->getBounds().contains(x, y)) {
                 return m_popup->onMouseDown(x - m_popup->getX(), y - m_popup->getY(), button, shift);
             } else {
                 closePopup();
                 return true;
             }
        }
        return Component::onMouseDown(x, y, button, shift);
    }
    
    bool onMouseMove(float x, float y, bool shift) override {
        if (!m_isVisible) return false;
        if (m_popup) {
            return m_popup->onMouseMove(x - m_popup->getX(), y - m_popup->getY(), shift);
        }
        return Component::onMouseMove(x, y, shift);
    }

    void setTab(int index) {
        m_currentTab = index;
        
        // CRITICAL FIX: explicit 'false' to prevent recursion loop
        if (m_generalTabBtn) m_generalTabBtn->setToggleState(index == 0, false);
        if (m_audioTabBtn) m_audioTabBtn->setToggleState(index == 1, false);
        
        bool g = (index == 0);
        bool a = (index == 1);
        
        if (m_autosaveToggle) m_autosaveToggle->setVisible(g);
        if (m_intervalSlider) m_intervalSlider->setVisible(g);
        
        if (m_sampleRateCombo) m_sampleRateCombo->setVisible(a);
        if (m_bufferSizeCombo) m_bufferSizeCombo->setVisible(a);
        if (m_scanPluginsBtn) m_scanPluginsBtn->setVisible(a);
        
        resized(); // Force layout update
    }
    
    void resized() override {
        // Tabs
        if (m_generalTabBtn) m_generalTabBtn->setBounds(20, 40, 100, 24);
        if (m_audioTabBtn) m_audioTabBtn->setBounds(130, 40, 100, 24);
        
        float contentY = 80;
        float controlX = 220;
        
        // General
        if (m_currentTab == 0) {
            if (m_autosaveToggle) m_autosaveToggle->setBounds(controlX, contentY, 60, 20);
            if (m_intervalSlider) m_intervalSlider->setBounds(controlX, contentY + 30, 180, 20);
        }
        
        // Audio
        if (m_currentTab == 1) {
             m_sampleRateCombo->setBounds(controlX, contentY, 150, 20);
             m_bufferSizeCombo->setBounds(controlX, contentY + 30, 150, 20);
             m_scanPluginsBtn->setBounds(controlX, contentY + 60, 150, 24);
        }
        
        if (m_closeBtn) m_closeBtn->setBounds(m_bounds.w - 120, m_bounds.h - 40, 100, 30);
    }

    void paint(QuadBatcher& batcher) override {
        // Modal Background
        batcher.drawRoundedRect(0, 0, m_bounds.w, m_bounds.h, 8.0f, 0.5f, 
                                Theme::Console.r, Theme::Console.g, Theme::Console.b, 1.0f);
        batcher.drawRect(0, 0, m_bounds.w, m_bounds.h, 2.0f, Theme::GreyLight.r, Theme::GreyLight.g, Theme::GreyLight.b, 1.0f);
        
        // Title
        batcher.drawText("PREFERENCES", 20, 15, 14, 1.0f, 1.0f, 1.0f, 1.0f);
        
        // Labels
        float contentY = 80;
        if (m_currentTab == 0) {
            batcher.drawText("Enable Autosave", 20, contentY + 4, 12, 0.9f, 0.9f, 0.9f, 1.0f);
            batcher.drawText("Interval (min)", 20, contentY + 34, 12, 0.9f, 0.9f, 0.9f, 1.0f);
        } else if (m_currentTab == 1) {
            batcher.drawText("Sample Rate", 20, contentY + 4, 12, 0.9f, 0.9f, 0.9f, 1.0f);
            batcher.drawText("Buffer Size", 20, contentY + 34, 12, 0.9f, 0.9f, 0.9f, 1.0f);
        }
        if (m_popup) {
             m_popup->render(batcher, 0.016f, m_bounds.w, m_bounds.h);
        }
    }

    std::function<void()> onClose;

private:
    BeamHost* m_host;
    AudioDeviceManager* m_deviceManager;
    int m_currentTab = 0;
    bool m_isLoading = false;

    std::shared_ptr<Button> m_generalTabBtn;
    std::shared_ptr<Button> m_audioTabBtn;
    std::shared_ptr<Button> m_closeBtn;
    
    // General
    std::shared_ptr<Button> m_autosaveToggle;
    std::shared_ptr<Slider> m_intervalSlider;

    // Audio
    std::shared_ptr<ComboBox> m_sampleRateCombo;
    std::shared_ptr<ComboBox> m_bufferSizeCombo;
    std::shared_ptr<Button> m_scanPluginsBtn;
    
    std::shared_ptr<Component> m_popup;
};

} // namespace Beam

#endif // SETTINGS_MODAL_HPP
