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
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 10.0f, 1.0f, 0.18f, 0.19f, 0.2f, 1.0f);
        // Header
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, 30, 10.0f, 0.5f, 0.25f, 0.26f, 0.28f, 1.0f);
        batcher.drawText("SPECTRUM ANALYZER", m_bounds.x + 10, m_bounds.y + 8, 12, 0.9f, 0.9f, 0.9f, 1.0f);

        // Delete Button (X) - Copying logic from AudioModule since we override render
        float dbX = m_bounds.x + m_bounds.w - 20;
        float dbY = m_bounds.y + 5;
        batcher.drawRoundedRect(dbX, dbY, 15, 15, 2.0f, 0.5f, 0.6f, 0.2f, 0.2f, 1.0f);
        batcher.drawText("x", dbX + 4, dbY + 2, 10, 1.0f, 1.0f, 1.0f, 1.0f);

        // Spectrum Graph Area
        float graphX = m_bounds.x + 10;
        float graphY = m_bounds.y + 40;
        float graphW = m_bounds.w - 20;
        float graphH = m_bounds.h - 50;
        
        batcher.drawRoundedRect(graphX, graphY, graphW, graphH, 5.0f, 2.0f, 0.05f, 0.05f, 0.05f, 1.0f);

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
                float db = param->getValue(); 
                float norm = (db + 60.0f) / 60.0f;
                norm = std::clamp(norm, 0.0f, 1.0f);
                
                float barH = norm * (graphH - 10);
                float bx = graphX + i * barWidth + 1;
                float by = graphY + graphH - barH - 5;
                
                float r = norm * norm; 
                float g = 1.0f - (norm * 0.5f);
                batcher.drawQuad(bx, by, barWidth - 2, barH, r, g, 0.2f, 0.9f);
                
                if (i % 2 == 0) {
                    batcher.drawText(freqs[i].substr(0, freqs[i].length()-2), bx, graphY + graphH - 8, 8, 0.5f, 0.5f, 0.5f, 1.0f);
                }
            }
        }
        
        // Ports - Accessing protected members
        if (m_inputPort) m_inputPort->render(batcher, dt, screenW, screenH);
        if (m_outputPort) m_outputPort->render(batcher, dt, screenW, screenH);
    }
};

} // namespace Beam

#endif
