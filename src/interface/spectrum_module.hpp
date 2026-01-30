#ifndef SPECTRUM_MODULE_HPP
#define SPECTRUM_MODULE_HPP

#include "audio_module.hpp"
#include <vector>
#include <string>
#include <algorithm>

namespace Beam {

class SpectrumModule : public AudioModule {
public:
    SpectrumModule(std::shared_ptr<FluxNode> node, size_t nodeId, float x, float y)
        : AudioModule(node, nodeId, x, y) {
        
        // Force ensure ports exist
        if (!m_inputPort) m_inputPort = std::make_shared<Port>(PortType::Input, this);
        if (!m_outputPort) m_outputPort = std::make_shared<Port>(PortType::Output, this);
        
        setBounds(x, y, 240, 180); 
    }

    void setBounds(float x, float y, float w, float h) override {
        AudioModule::setBounds(x, y, w, h);
        if (m_inputPort) m_inputPort->setBounds(x + 10, y + 10, 12, 12);
        if (m_outputPort) m_outputPort->setBounds(x + w - 22, y + 10, 12, 12);
    }

    void paint(QuadBatcher& batcher) override;

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        Component::render(batcher, dt, screenW, screenH);
    }

    void drawSpectrumGraph(QuadBatcher& batcher) {
        float graphX = m_bounds.x + 10;
        float graphY = m_bounds.y + 40;
        float graphW = m_bounds.w - 20;
        float graphH = m_bounds.h - 50;
        
        batcher.drawRoundedRect(graphX, graphY, graphW, graphH, 5.0f, 2.0f, 0.05f, 0.05f, 0.05f, 1.0f);

        for(float y=0; y<=1.0f; y+=0.25f) {
            batcher.drawQuad(graphX, graphY + y * graphH, graphW, 1, 0.2f, 0.2f, 0.2f, 0.5f);
        }

        std::vector<std::string> freqs = {"31Hz", "63Hz", "125Hz", "250Hz", "500Hz", "1000Hz", "2000Hz", "4000Hz", "8000Hz", "16000Hz"};
        float barWidth = graphW / (float)freqs.size();
        
        for (size_t i = 0; i < freqs.size(); ++i) {
            auto param = m_node->getParameter(freqs[i]);
            if (param) {
                float db = param->getValue(); 
                float norm = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
                
                float barH = norm * (graphH - 10);
                float bx = graphX + i * barWidth + 1;
                float by = graphY + graphH - barH - 5;
                
                batcher.drawRoundedGradientRect(bx, by, barWidth - 2, barH, 1.0f, 0.5f, 0.13f, 0.62f, 0.42f, 0.9f, 0.05f, 0.05f, 0.06f, 0.9f);
            }
        }
    }
};

} // namespace Beam

#endif