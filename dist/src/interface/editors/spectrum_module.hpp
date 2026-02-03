#ifndef SPECTRUM_MODULE_HPP
#define SPECTRUM_MODULE_HPP

#include "interface/modules/audio_module.hpp"
#include "interface/core/theme.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include "interface/widgets/spectrum_display.hpp"

namespace Beam {

class SpectrumModule : public AudioModule {
public:
    SpectrumModule(std::shared_ptr<FluxNode> node, size_t nodeId, float x, float y)
        : AudioModule(node, nodeId, x, y) {
        
        // Force ensure ports exist
        if (m_inputPorts.empty()) {
            auto port = std::make_shared<Port>(PortType::Input, this, 0);
            m_inputPorts.push_back(port);
            addChildComponent(port);
        }
        if (m_outputPorts.empty()) {
            auto port = std::make_shared<Port>(PortType::Output, this, 0);
            m_outputPorts.push_back(port);
            addChildComponent(port);
        }
        
        m_display = std::make_shared<SpectrumDisplay>();
        addChildComponent(m_display);
        
        setBounds(x, y, 480, 240); // Increased Size
    }

    void setBounds(float x, float y, float w, float h) override {
        AudioModule::setBounds(x, y, w, h);
        if (!m_inputPorts.empty()) m_inputPorts[0]->setBounds(x + 10, y + 10, 12, 12);
        if (!m_outputPorts.empty()) m_outputPorts[0]->setBounds(x + w - 22, y + 10, 12, 12);
        
        // Graph area
        float graphX = 10;
        float graphY = 40;
        float graphW = w - 20;
        float graphH = h - 50;
        m_display->setBounds(graphX, graphY, graphW, graphH);
    }

    void paint(QuadBatcher& batcher) override;

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (m_node) {
            m_node->updateVisuals();
            updateSpectrumData();
        }
        Component::render(batcher, dt, screenW, screenH);
    }
    
private:
    std::shared_ptr<SpectrumDisplay> m_display;
    std::vector<float> m_spectrumData;

    void updateSpectrumData() {
        // ... (Using same logic as SpectrumEditor) ...
        // Simplified Logic: 10 bands from params (updated by node) -> interpolated
        std::vector<std::string> freqs = {"31Hz", "63Hz", "125Hz", "250Hz", "500Hz", "1000Hz", "2000Hz", "4000Hz", "8000Hz", "16000Hz"};
        std::vector<float> bandGains;
        for (const auto& f : freqs) {
            auto p = m_node->getParameter(f);
            float val = p ? p->getValue() : -60.0f; // in dB
            bandGains.push_back(std::pow(10.0f, val / 20.0f));
        }
        
        size_t numPoints = 512;
        if (m_spectrumData.size() != numPoints) m_spectrumData.resize(numPoints);
        
        for (size_t i = 0; i < numPoints; ++i) {
            float t = (float)i / (float)(numPoints - 1);
            float bandPos = t * (bandGains.size() - 1);
            size_t idx = (size_t)bandPos;
            float frac = bandPos - idx;
            float v1 = bandGains[idx];
            float v2 = (idx + 1 < bandGains.size()) ? bandGains[idx+1] : v1;
            float val = v1 * (1.0f - frac) + v2 * frac; // Linear interp
            m_spectrumData[i] = val;
        }
        m_display->setFrequencyData(m_spectrumData, 44100.0f);
    }
};

} // namespace Beam

#endif