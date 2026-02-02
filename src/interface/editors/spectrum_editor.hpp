#ifndef SPECTRUM_EDITOR_HPP
#define SPECTRUM_EDITOR_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/plugins/flux_plugin.hpp"
#include <vector>
#include <string>
#include <algorithm>

namespace Beam {

class SpectrumEditor : public Component {
public:
    SpectrumEditor(FluxPlugin* node) : m_node(node) {
        setName("SpectrumEditor");
    }

    void getPreferredSize(float& w, float& h) const override {
        w = 240; 
        h = 100;
    }

    void paint(QuadBatcher& batcher) override {
        float graphW = m_bounds.w;
        float graphH = m_bounds.h;
        float graphX = 0;
        float graphY = 0;

        batcher.drawRoundedRect(graphX, graphY, graphW, graphH, 5.0f, 2.0f, 0.05f, 0.05f, 0.05f, 1.0f);

        for(float y=0; y<=1.0f; y+=0.25f) {
             batcher.drawQuad(graphX, graphY + y * graphH, graphW, 1, 0.2f, 0.2f, 0.2f, 0.5f);
        }

        if (!m_node) return;

        std::vector<std::string> freqs = {"31Hz", "63Hz", "125Hz", "250Hz", "500Hz", "1000Hz", "2000Hz", "4000Hz", "8000Hz", "16000Hz"};
        float barWidth = graphW / (float)freqs.size();
        
        for (size_t i = 0; i < freqs.size(); ++i) {
            auto param = m_node->getParameter(freqs[i]);
            if (param) {
                float db = param->getValue(); 
                float norm = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);
                
                float barH = norm * graphH;
                float bx = graphX + i * barWidth;
                float by = graphY + graphH - barH;
                batcher.drawRoundedGradientRect(bx, by, barWidth - 2, barH, 1.0f, 0.5f, 
                                               Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 0.9f, 
                                               Theme::Black.r, Theme::Black.g, Theme::Black.b, 0.9f);
            }
        }
    }
private:
    FluxPlugin* m_node;
};

} // namespace Beam
#endif
