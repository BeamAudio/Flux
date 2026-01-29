#ifndef AUDIO_CONFIG_VIEW_HPP
#define AUDIO_CONFIG_VIEW_HPP

#include "component.hpp"
#include "../engine/audio_device_manager.hpp"
#include <vector>
#include <string>

namespace Beam {

class AudioConfigView : public Component {
public:
    AudioConfigView(AudioDeviceManager* manager, class AudioEngine* engine) 
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

    void render(QuadBatcher& batcher, float dt) override {
        if (!m_isVisible) return;

        // Dim background
        batcher.drawQuad(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 0.05f, 0.05f, 0.06f, 0.95f);
        
        float xOff = m_bounds.x + 50;
        float yOff = m_bounds.y + 50;

        batcher.drawText("AUDIO CONFIGURATION", xOff, yOff, 20, 1.0f, 1.0f, 1.0f, 1.0f);
        
        if (m_engine && m_engine->isPlaying()) {
            batcher.drawText("!! PAUSE PLAYBACK TO CHANGE SETTINGS !!", xOff + 250, yOff, 14, 1.0f, 0.2f, 0.2f, 1.0f);
        }

        yOff += 40;

        // Output Devices
        batcher.drawText("Output Device:", xOff, yOff, 14, 0.7f, 0.7f, 0.7f, 1.0f);
        yOff += 25;
        for (const auto& dev : m_outputDevices) {
            bool isSelected = dev.name == m_currentSetup.outputDeviceName;
            batcher.drawRoundedRect(xOff, yOff, 300, 25, 4.0f, 0.5f, isSelected ? 0.2f : 0.15f, isSelected ? 0.4f : 0.16f, isSelected ? 0.8f : 0.17f, 1.0f);
            batcher.drawText(dev.name, xOff + 10, yOff + 6, 12, 0.9f, 0.9f, 0.9f, 1.0f);
            yOff += 30;
        }

        yOff += 20;
        // Input Devices
        batcher.drawText("Input Device:", xOff, yOff, 14, 0.7f, 0.7f, 0.7f, 1.0f);
        yOff += 25;
        for (const auto& dev : m_inputDevices) {
            bool isSelected = dev.name == m_currentSetup.inputDeviceName;
            batcher.drawRoundedRect(xOff, yOff, 300, 25, 4.0f, 0.5f, isSelected ? 0.2f : 0.15f, isSelected ? 0.4f : 0.16f, isSelected ? 0.8f : 0.17f, 1.0f);
            batcher.drawText(dev.name, xOff + 10, yOff + 6, 12, 0.9f, 0.9f, 0.9f, 1.0f);
            yOff += 30;
        }

        yOff += 20;
        // Sample Rate
        batcher.drawText("Sample Rate:", xOff, yOff, 14, 0.7f, 0.7f, 0.7f, 1.0f);
        yOff += 25;
        std::vector<double> rates = {44100.0, 48000.0, 88200.0, 96000.0};
        float rx = xOff;
        for (double r : rates) {
            bool isSelected = std::abs(r - m_currentSetup.sampleRate) < 1.0;
            batcher.drawRoundedRect(rx, yOff, 80, 25, 4.0f, 0.5f, isSelected ? 0.2f : 0.15f, isSelected ? 0.4f : 0.16f, isSelected ? 0.8f : 0.17f, 1.0f);
            batcher.drawText(std::to_string((int)r), rx + 10, yOff + 6, 12, 0.9f, 0.9f, 0.9f, 1.0f);
            rx += 90;
        }

        yOff += 40;
        // Buffer Size
        batcher.drawText("Buffer Size:", xOff, yOff, 14, 0.7f, 0.7f, 0.7f, 1.0f);
        yOff += 25;
        std::vector<int> sizes = {128, 256, 512, 1024, 2048};
        rx = xOff;
        for (int s : sizes) {
            bool isSelected = s == m_currentSetup.blockSize;
            batcher.drawRoundedRect(rx, yOff, 80, 25, 4.0f, 0.5f, isSelected ? 0.2f : 0.15f, isSelected ? 0.4f : 0.16f, isSelected ? 0.8f : 0.17f, 1.0f);
            batcher.drawText(std::to_string(s), rx + 15, yOff + 6, 12, 0.9f, 0.9f, 0.9f, 1.0f);
            rx += 90;
        }

        // Close Button
        batcher.drawRoundedRect(m_bounds.x + m_bounds.w - 100, m_bounds.y + 50, 50, 30, 4.0f, 0.5f, 0.6f, 0.2f, 0.2f, 1.0f);
        batcher.drawText("CLOSE", m_bounds.x + m_bounds.w - 95, m_bounds.y + 58, 12, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    bool onMouseDown(float x, float y, int button) override {
        if (!m_isVisible) return false;

        // Close button (always allowed)
        if (x > m_bounds.x + m_bounds.w - 100 && x < m_bounds.x + m_bounds.w - 50 && y > m_bounds.y + 50 && y < m_bounds.y + 80) {
            m_isVisible = false;
            return true;
        }

        if (m_engine && m_engine->isPlaying()) {
            return true; // Ignore setting changes but eat click
        }

        float xOff = m_bounds.x + 50;
        float yOff = m_bounds.y + 90;

        // Output device clicks
        for (const auto& dev : m_outputDevices) {
            if (x > xOff && x < xOff + 300 && y > yOff && y < yOff + 25) {
                m_currentSetup.outputDeviceName = dev.name;
                apply();
                return true;
            }
            yOff += 30;
        }

        yOff += 45;
        // Input device clicks
        for (const auto& dev : m_inputDevices) {
            if (x > xOff && x < xOff + 300 && y > yOff && y < yOff + 25) {
                m_currentSetup.inputDeviceName = dev.name;
                apply();
                return true;
            }
            yOff += 30;
        }

        yOff += 45;
        // Sample Rate clicks
        float rx = xOff;
        std::vector<double> rates = {44100.0, 48000.0, 88200.0, 96000.0};
        for (double r : rates) {
            if (x > rx && x < rx + 80 && y > yOff && y < yOff + 25) {
                m_currentSetup.sampleRate = r;
                apply();
                return true;
            }
            rx += 90;
        }

        yOff += 65;
        // Buffer Size clicks
        rx = xOff;
        std::vector<int> sizes = {128, 256, 512, 1024, 2048};
        for (int s : sizes) {
            if (x > rx && x < rx + 80 && y > yOff && y < yOff + 25) {
                m_currentSetup.blockSize = s;
                apply();
                return true;
            }
            rx += 90;
        }

        return true; // Eat clicks while open
    }

    void setVisible(bool visible) { 
        m_isVisible = visible; 
        if (visible) refreshDevices();
    }

    bool isVisible() const { return m_isVisible; }

    void apply() {
        if (m_manager) {
            m_manager->setCurrentAudioDevice(m_currentSetup.outputDeviceName, m_currentSetup.inputDeviceName, m_currentSetup.sampleRate, m_currentSetup.blockSize);
            if (onConfigChanged) onConfigChanged();
        }
    }

    std::function<void()> onConfigChanged;

private:
    AudioDeviceManager* m_manager;
    class AudioEngine* m_engine;
    std::vector<AudioDeviceInfo> m_outputDevices;
    std::vector<AudioDeviceInfo> m_inputDevices;
    AudioDeviceSetup m_currentSetup;
    bool m_isVisible = false;
};

} // namespace Beam

#endif // AUDIO_CONFIG_VIEW_HPP


