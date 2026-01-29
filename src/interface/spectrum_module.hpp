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
        setBounds(x, y, 220, 160); // Bigger than standard module
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Background
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 6.0f, 1.0f, 0.1f, 0.1f, 0.12f, 1.0f);
        
        // Header
        batcher.drawText("SPECTRUM", m_bounds.x + 10, m_bounds.y + 10, 12, 0.8f, 0.8f, 0.8f, 1.0f);

        // Spectrum Graph
        float graphX = m_bounds.x + 10;
        float graphY = m_bounds.y + 30;
        float graphW = m_bounds.w - 20;
        float graphH = m_bounds.h - 40;
        
        batcher.drawQuad(graphX, graphY, graphW, graphH, 0.05f, 0.05f, 0.05f, 1.0f); // Graph BG

        // Grid lines
        for(float y=0; y<=1.0f; y+=0.25f) {
            batcher.drawQuad(graphX, graphY + y * graphH, graphW, 1, 0.2f, 0.2f, 0.2f, 0.5f);
        }

        // Frequencies provided by FluxSpectrumAnalyzer
        std::vector<std::string> freqs = {"31Hz", "63Hz", "125Hz", "250Hz", "500Hz", "1000Hz", "2000Hz", "4000Hz", "8000Hz", "16000Hz"};
        
        float barWidth = graphW / (float)freqs.size();
        
        for (size_t i = 0; i < freqs.size(); ++i) {
            auto param = m_node->getParameter(freqs[i]);
            if (param) {
                float db = param->getValue(); // -60 to 0 (approx)
                // Normalize -60..0 to 0..1
                float norm = (db + 60.0f) / 60.0f;
                norm = std::clamp(norm, 0.0f, 1.0f);
                
                float barH = norm * graphH;
                float bx = graphX + i * barWidth + 1;
                float by = graphY + graphH - barH;
                
                // Color gradient based on intensity (Green to Red)
                float r = norm * norm; 
                float g = 1.0f - (norm * 0.5f);
                float b = 0.2f;
                
                batcher.drawQuad(bx, by, barWidth - 2, barH, r, g, b, 0.9f);
                
                // Label every other
                if (i % 2 == 0) {
                    batcher.drawText(freqs[i].substr(0, freqs[i].length()-2), bx, graphY + graphH + 2, 8, 0.5f, 0.5f, 0.5f, 1.0f);
                }
            }
        }
        
        // Input/Output Ports
        if (getInputPort()) getInputPort()->render(batcher, dt, screenW, screenH);
        if (getOutputPort()) getOutputPort()->render(batcher, dt, screenW, screenH);
    }
};

} // namespace Beam

#endif
