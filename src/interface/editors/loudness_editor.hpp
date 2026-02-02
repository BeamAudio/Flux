#ifndef LOUDNESS_EDITOR_HPP
#define LOUDNESS_EDITOR_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/plugins/flux_plugin.hpp"
#include <string>
#include <algorithm>

namespace Beam {

class LoudnessEditor : public Component {
public:
    LoudnessEditor(FluxPlugin* node) : m_node(node) {
        setName("LoudnessEditor");
    }

    void getPreferredSize(float& w, float& h) const override {
        w = 180;
        h = 120;
    }

    void paint(QuadBatcher& batcher) override {
        if (!m_node) return;

        float graphW = m_bounds.w;
        float graphH = m_bounds.h; // Use full bounds
        float x = 0;
        float y = 0;

        auto pM = m_node->getParameter("Momentary");
        auto pS = m_node->getParameter("ShortTerm");
        auto pT = m_node->getParameter("True Peak");

        auto drawBar = [&](const std::string& label, float db, float bx, float bw, float r, float g, float b) {
            float norm = (db + 60.0f) / 60.0f;
            norm = std::clamp(norm, 0.0f, 1.0f);
            
            // Draw background track
            batcher.drawRoundedRect(bx, y, bw, graphH, 2.0f, 0.5f, 0.2f, 0.2f, 0.2f, 1.0f);

            float h = norm * graphH;
            batcher.drawRoundedRect(bx, y + graphH - h, bw, h, 2.0f, 0.5f, r, g, b, 1.0f);
            
            // Label
            batcher.drawText(label, bx + 5, y + graphH + 12, 10, 0.7f, 0.7f, 0.7f, 1.0f);
            
            std::string valStr = std::to_string((int)db);
            batcher.drawText(valStr, bx + 5, y + graphH - 12, 10, 1.0f, 1.0f, 1.0f, 1.0f);
        };
        
        float pad = 10;
        float colW = 40;
        
        if (pM) drawBar("M", pM->getValue(), x + pad, colW, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b); 
        if (pS) drawBar("S", pS->getValue(), x + pad + colW + 10, colW, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b); 
        if (pT) drawBar("TP", pT->getValue(), x + pad + (colW + 10)*2, colW, Theme::Red.r, Theme::Red.g, Theme::Red.b); 
    }

private:
    FluxPlugin* m_node;
};

} // namespace Beam
#endif
