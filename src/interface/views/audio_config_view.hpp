#ifndef AUDIO_CONFIG_VIEW_HPP
#define AUDIO_CONFIG_VIEW_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "interface/widgets/combo_box.hpp"
#include "engine/core/audio_device_manager.hpp"
#include "interface/popup/popup_host.hpp"
#include <vector>
#include <string>
#include <cmath>

namespace Beam {

class AudioConfigView : public Component, public PopupHost {
public:
    AudioConfigView(class BeamHost* host, AudioDeviceManager* manager, AudioEngine* engine) 
        : m_host(host), m_manager(manager), m_engine(engine) {
        setName("AudioConfigView");
        setVisible(false);
        
        m_sampleRateCombo = std::make_shared<ComboBox>();
        m_bufferSizeCombo = std::make_shared<ComboBox>();
        
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
        
        m_sampleRateCombo->setOnChange([this](int idx) { applyConfig(); });
        m_bufferSizeCombo->setOnChange([this](int idx) { applyConfig(); });
    }

    void refreshDevices() {
        if (!m_manager) return;
        m_currentSetup = m_manager->getCurrentDeviceSetup();
        
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
            m_currentSetup.outputDeviceName, m_currentSetup.outputDeviceId,
            m_currentSetup.inputDeviceName, m_currentSetup.inputDeviceId,
            sr, buf);
            
        if (onConfigChanged) onConfigChanged();
    }

    void showPopup(std::shared_ptr<Component> popup) override {
        m_popup = popup;
        if(m_popup) {
            m_popup->setParent(this);
            // Convert Global Screen Coords to Local for the popup placement
            // ComboBox calculates Global Bounds. We need to convert back?
            // Actually PopupMenu expects to be drawn in Global space or overlay?
            // In Workspace, we did: batcher.resetViewTransform...
            // Here, AudioConfigView is rendered in standard space.
            // But ComboBox::showPopup uses getScreenBounds() to set bounds.
            // We need to ensure paint() handles this.
        }
    }

    void closePopup() override {
        m_popup = nullptr;
    }

    void paint(QuadBatcher& batcher) override {
        if (!m_isVisible) return;

        // Overlay (Local 0,0)
        batcher.drawQuad(0, 0, m_bounds.w, m_bounds.h, 0.0f, 0.0f, 0.0f, 0.7f);
        
        float winW = 600;
        float winH = 400;
        float winX = (m_bounds.w - winW) * 0.5f;
        float winY = (m_bounds.h - winH) * 0.5f;

        // Chassis Window
        batcher.drawChassisPanel(winX, winY, winW, winH, 12.0f, Theme::Console.r, Theme::Console.g, Theme::Console.b, 1.0f);
        
        // Header
        batcher.drawRoundedGradientRect(winX + 5, winY + 5, winW - 10, 40, 8.0f, 0.5f, 
                                       Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 1.0f,
                                       Theme::Emerald.darker(0.3f).r, Theme::Emerald.darker(0.3f).g, Theme::Emerald.darker(0.3f).b, 1.0f);
                                       
        batcher.drawText("AUDIO CONFIGURATION", winX + 25, winY + 14, 16, 1.0f, 1.0f, 1.0f, 1.0f);

        // Labels
        float xOff = winX + 30;
        float yOff = winY + 80;

        auto drawLabel = [&](const std::string& text, float y) {
            batcher.drawText(text, xOff, y, 14, 0.9f, 0.9f, 0.9f, 1.0f);
        };

        drawLabel("Sample Rate:", yOff);
        drawLabel("Buffer Size:", yOff + 60);

        // Close Button
        float closeBtnX = winX + winW - 40;
        float closeBtnY = winY + 12;
        batcher.drawRoundedRect(closeBtnX, closeBtnY, 25, 25, 4.0f, 0.5f, Theme::Red.r, Theme::Red.g, Theme::Red.b, 1.0f);
        batcher.drawText("X", closeBtnX + 8, closeBtnY + 6, 14, 1.0f, 1.0f, 1.0f, 1.0f);
        
        // Render Popup if active
        if (m_popup) {
            // Popup bounds are in Screen Coordinates (set by ComboBox)
            // But we are rendering in AudioConfigView's Local space (which is Screen Space mostly, but might be offset?)
            // AudioConfigView is usually (0,0) width/height of screen.
            // If so, Screen Bounds == Local Bounds.
            m_popup->render(batcher, 0.016f, m_bounds.w, m_bounds.h);
        }
    }

    void resized() override {
        float winW = 600;
        float winH = 400;
        float winX = (m_bounds.w - winW) * 0.5f;
        float winY = (m_bounds.h - winH) * 0.5f;
        float xOff = winX + 150;
        float yOff = winY + 80;
        
        m_sampleRateCombo->setBounds(xOff, yOff, 150, 24);
        m_bufferSizeCombo->setBounds(xOff, yOff + 60, 150, 24);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        Component::render(batcher, dt, screenW, screenH);
    }

    bool onMouseDown(float x, float y, int button, bool shift) override {
        if (!m_isVisible) return false;
        
        // Popup Handling
        if (m_popup) {
            if (m_popup->getBounds().contains(x, y)) {
                return m_popup->onMouseDown(x - m_popup->getX(), y - m_popup->getY(), button, shift);
            } else {
                closePopup();
                return true;
            }
        }
        
        // Window Calculation
        float winW = 600;
        float winH = 400;
        float winX = (m_bounds.w - winW) * 0.5f;
        float winY = (m_bounds.h - winH) * 0.5f;
        
        // 1. Close Button Hit Test (Priority!)
        // Coordinates for Close Button relative to Component
        float closeX = winX + winW - 40;
        float closeY = winY + 12;
        float closeW = 25;
        float closeH = 25;
        
        if (x >= closeX && x <= closeX + closeW && y >= closeY && y <= closeY + closeH) {
            setVisible(false);
            return true;
        }

        // 2. Children (ComboBoxes)
        // Manual dispatch to avoid greedy base implementation
        float localX = x - m_bounds.x;
        float localY = y - m_bounds.y;
        for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
             if ((*it)->onMouseDown(localX, localY, button, shift)) return true;
        }

        // 3. Window Background (Consume click)
        if (x >= winX && x <= winX + winW && y >= winY && y <= winY + winH) return true;
        
        // 4. Click outside? Close.
        setVisible(false);
        return true; 
    }

    bool onMouseMove(float x, float y, bool shift) override {
        if (!m_isVisible) return false;
        if (m_popup) {
            return m_popup->onMouseMove(x - m_popup->getX(), y - m_popup->getY(), shift);
        }
        return Component::onMouseMove(x, y, shift);
    }

    void setVisible(bool visible) override { 
        Component::setVisible(visible);
        if (visible) refreshDevices();
    }

    std::function<void()> onConfigChanged;

private:
    class BeamHost* m_host;
    AudioDeviceManager* m_manager;
    AudioEngine* m_engine;
    AudioDeviceSetup m_currentSetup;
    
    std::shared_ptr<ComboBox> m_sampleRateCombo;
    std::shared_ptr<ComboBox> m_bufferSizeCombo;
    std::shared_ptr<Component> m_popup;
};

} // namespace Beam

#endif