#ifndef AUDIO_CONFIG_VIEW_HPP
#define AUDIO_CONFIG_VIEW_HPP

#include "component.hpp"
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
        m_isVisible = false;
        refreshDevices();
    }

    void refreshDevices() {
        if (!m_manager) return;
        m_outputDevices = m_manager->getAvailableOutputDevices();
        m_inputDevices = m_manager->getAvailableInputDevices();
        m_currentSetup = m_manager->getCurrentDeviceSetup();
    }

    bool isVisible() const { return m_isVisible; }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_isVisible) return;

        // 1. Dim the background
        batcher.drawQuad(0, 0, 5000, 5000, 0.0f, 0.0f, 0.0f, 0.7f);
        
        // 2. Centered Window Box
        float winW = 600;
        float winH = 500;
        float winX = m_bounds.x + (m_bounds.w - winW) * 0.5f;
        float winY = m_bounds.y + (m_bounds.h - winH) * 0.5f;

        // Shadow
        batcher.drawRoundedRect(winX + 5, winY + 5, winW, winH, 12.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.5f);
        // Window Body
        batcher.drawRoundedRect(winX, winY, winW, winH, 12.0f, 1.0f, 0.15f, 0.15f, 0.17f, 1.0f);
        // Title Bar
        batcher.drawRoundedRect(winX, winY, winW, 35, 12.0f, 0.5f, 0.22f, 0.22f, 0.25f, 1.0f);
        batcher.drawText("AUDIO SETTINGS", winX + 20, winY + 10, 14, 1.0f, 1.0f, 1.0f, 1.0f);

        float xOff = winX + 30;
        float yOff = winY + 60;

        if (m_engine && m_engine->isPlaying()) {
            batcher.drawRoundedRect(winX + 10, yOff - 5, winW - 20, 30, 4.0f, 1.0f, 0.4f, 0.1f, 0.1f, 1.0f);
            batcher.drawText("!! STOP PLAYBACK TO MODIFY SETTINGS !!", winX + 120, yOff + 5, 14, 1.0f, 0.8f, 0.8f, 1.0f);
            yOff += 45;
        }

        // Output Devices
        batcher.drawText("Output Device:", xOff, yOff, 12, 0.6f, 0.6f, 0.6f, 1.0f);
        yOff += 20;
        for (const auto& dev : m_outputDevices) {
            bool isSelected = dev.name == m_currentSetup.outputDeviceName;
            batcher.drawRoundedRect(xOff, yOff, 350, 24, 4.0f, 0.5f, isSelected ? 0.25f : 0.18f, isSelected ? 0.45f : 0.19f, isSelected ? 0.85f : 0.2f, 1.0f);
            batcher.drawText(dev.name, xOff + 10, yOff + 6, 11, 0.9f, 0.9f, 0.9f, 1.0f);
            yOff += 28;
        }

        yOff += 15;
        // Input Devices
        batcher.drawText("Input Device:", xOff, yOff, 12, 0.6f, 0.6f, 0.6f, 1.0f);
        yOff += 20;
        for (const auto& dev : m_inputDevices) {
            bool isSelected = dev.name == m_currentSetup.inputDeviceName;
            batcher.drawRoundedRect(xOff, yOff, 350, 24, 4.0f, 0.5f, isSelected ? 0.25f : 0.18f, isSelected ? 0.45f : 0.19f, isSelected ? 0.85f : 0.2f, 1.0f);
            batcher.drawText(dev.name, xOff + 10, yOff + 6, 11, 0.9f, 0.9f, 0.9f, 1.0f);
            yOff += 28;
        }

        yOff += 15;
        // Sample Rate
        batcher.drawText("Sample Rate:", xOff, yOff, 12, 0.6f, 0.6f, 0.6f, 1.0f);
        yOff += 20;
        std::vector<double> rates = {44100.0, 48000.0, 88200.0, 96000.0};
        float rx = xOff;
        for (double r : rates) {
            bool isSelected = std::abs(r - m_currentSetup.sampleRate) < 1.0;
            batcher.drawRoundedRect(rx, yOff, 70, 24, 4.0f, 0.5f, isSelected ? 0.25f : 0.18f, isSelected ? 0.45f : 0.19f, isSelected ? 0.85f : 0.2f, 1.0f);
            batcher.drawText(std::to_string((int)r), rx + 10, yOff + 6, 11, 0.9f, 0.9f, 0.9f, 1.0f);
            rx += 75;
        }

        yOff += 40;
        // Buffer Size
        batcher.drawText("Buffer Size:", xOff, yOff, 12, 0.6f, 0.6f, 0.6f, 1.0f);
        yOff += 20;
        std::vector<int> sizes = {128, 256, 512, 1024, 2048};
        rx = xOff;
        for (int s : sizes) {
            bool isSelected = s == m_currentSetup.blockSize;
            batcher.drawRoundedRect(rx, yOff, 70, 24, 4.0f, 0.5f, isSelected ? 0.25f : 0.18f, isSelected ? 0.45f : 0.19f, isSelected ? 0.85f : 0.2f, 1.0f);
            batcher.drawText(std::to_string(s), rx + 12, yOff + 6, 11, 0.9f, 0.9f, 0.9f, 1.0f);
            rx += 75;
        }

        // Close Button (Top Right)
        float closeBtnX = winX + winW - 40;
        float closeBtnY = winY + 7;
        batcher.drawRoundedRect(closeBtnX, closeBtnY, 25, 20, 4.0f, 0.5f, 0.6f, 0.2f, 0.2f, 1.0f);
        batcher.drawText("X", closeBtnX + 8, closeBtnY + 4, 12, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible) return false;

        float winW = 600;
        float winH = 500;
        float winX = m_bounds.x + (m_bounds.w - winW) * 0.5f;
        float winY = m_bounds.y + (m_bounds.h - winH) * 0.5f;

        // Close button check
        if (x > winX + winW - 45 && x < winX + winW && y > winY && y < winY + 35) {
            m_isVisible = false;
            return true;
        }

        // If playing, block all setting clicks
        if (m_engine && m_engine->isPlaying()) {
            return true; 
        }

        float xOff = winX + 30;
        float currentY = winY + 80;

        // Output device clicks
        for (const auto& dev : m_outputDevices) {
            if (x > xOff && x < xOff + 350 && y > currentY && y < currentY + 24) {
                m_currentSetup.outputDeviceName = dev.name;
                m_currentSetup.outputDeviceId = dev.deviceId;
                apply();
                return true;
            }
            currentY += 28;
        }

        currentY += 35;
        // Input device clicks
        for (const auto& dev : m_inputDevices) {
            if (x > xOff && x < xOff + 350 && y > currentY && y < currentY + 24) {
                m_currentSetup.inputDeviceName = dev.name;
                m_currentSetup.inputDeviceId = dev.deviceId;
                apply();
                return true;
            }
            currentY += 28;
        }

        currentY += 35;
        // Sample Rate clicks
        float rx = xOff;
        std::vector<double> rates = {44100.0, 48000.0, 88200.0, 96000.0};
        for (double r : rates) {
            if (x > rx && x < rx + 70 && y > currentY && y < currentY + 24) {
                m_currentSetup.sampleRate = r;
                apply();
                return true;
            }
            rx += 75;
        }

        currentY += 60;
        // Buffer Size clicks
        rx = xOff;
        std::vector<int> sizes = {128, 256, 512, 1024, 2048};
        for (int s : sizes) {
            if (x > rx && x < rx + 70 && y > currentY && y < currentY + 24) {
                m_currentSetup.blockSize = s;
                apply();
                return true;
            }
            rx += 75;
        }

        return true; // Modal behavior: absorb all clicks
    }

    void setVisible(bool visible) { 
        m_isVisible = visible; 
        if (visible) refreshDevices();
    }

    void apply() {
        if (m_manager) {
            m_manager->setCurrentAudioDevice(m_currentSetup.outputDeviceName, m_currentSetup.outputDeviceId,
                                             m_currentSetup.inputDeviceName, m_currentSetup.inputDeviceId,
                                             m_currentSetup.sampleRate, m_currentSetup.blockSize);
            if (onConfigChanged) onConfigChanged();
        }
    }

    std::function<void()> onConfigChanged;

private:
    AudioDeviceManager* m_manager;
    AudioEngine* m_engine;
    std::vector<AudioDeviceInfo> m_outputDevices;
    std::vector<AudioDeviceInfo> m_inputDevices;
    AudioDeviceSetup m_currentSetup;
    bool m_isVisible = false;
};

} // namespace Beam

#endif // AUDIO_CONFIG_VIEW_HPP


