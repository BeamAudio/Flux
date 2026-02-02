#ifndef LOUDNESS_MODULE_HPP
#define LOUDNESS_MODULE_HPP

#include "interface/modules/audio_module.hpp"
#include "interface/core/theme.hpp"
#include <string>

namespace Beam {

class LoudnessModule : public AudioModule {
public:
    LoudnessModule(std::shared_ptr<FluxNode> node, size_t nodeId, float x, float y)
        : AudioModule(node, nodeId, x, y) {
        setBounds(x, y, 180, 200);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        // Base module frame
        AudioModule::render(batcher, dt, screenW, screenH);

        // Custom visualization for loudness
        float graphX = m_bounds.x + 10;
        float graphY = m_bounds.y + 40;
        float graphW = m_bounds.w - 20;
        float graphH = m_bounds.h - 50;

        auto pM = m_node->getParameter("Momentary");
        auto pS = m_node->getParameter("ShortTerm");
        auto pT = m_node->getParameter("True Peak");

        auto drawBar = [&](const std::string& label, float db, float x, float w, float r, float g, float b) {
            float norm = (db + 60.0f) / 60.0f;
            norm = std::clamp(norm, 0.0f, 1.0f);
            float h = norm * (graphH - 20);
            batcher.drawQuad(x, graphY + graphH - h - 15, w, h, r, g, b, 0.8f);
            batcher.drawText(label, x, graphY + graphH - 12, 8, 0.7f, 0.7f, 0.7f, 1.0f);
            
            std::string valStr = std::to_string((int)db) + " dB";
            batcher.drawText(valStr, x, graphY + graphH - h - 25, 8, 0.9f, 0.9f, 0.9f, 1.0f);
        };

        if (pM) drawBar("M", pM->getValue(), graphX + 10, 30, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b); 
        if (pS) drawBar("S", pS->getValue(), graphX + 50, 30, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b); 
        if (pT) drawBar("TP", pT->getValue(), graphX + 90, 30, Theme::Red.r, Theme::Red.g, Theme::Red.b); 
    }
};

} // namespace Beam

#endif
