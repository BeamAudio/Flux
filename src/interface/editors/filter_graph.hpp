#ifndef FILTER_GRAPH_HPP
#define FILTER_GRAPH_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/nodes/biquad_filter_node.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace Beam {

class FilterGraph : public Component {
public:
    FilterGraph(BiquadFilterNode* node) : m_node(node) {
        setName("FilterGraph");
        setBounds(0, 0, 130, 80);
    }

    void paint(QuadBatcher& batcher) override {
        // Background (Local 0,0)
        batcher.drawRoundedRect(0, 0, m_bounds.w, m_bounds.h, 4.0f, 1.0f, Theme::Black.r, Theme::Black.g, Theme::Black.b, 1.0f);
        
        // Grid lines
        for (float f = 0.1f; f < 1.0f; f += 0.2f) {
            float lx = f * m_bounds.w;
            batcher.drawQuad(lx, 0, 1, m_bounds.h, 0.15f, 0.15f, 0.15f, 1.0f);
        }

        if (!m_node) return;

        std::vector<std::pair<float, float>> points;
        const int numPoints = 40;
        float step = m_bounds.w / (float)numPoints;
        
        for (int i = 0; i <= numPoints; ++i) {
            float normFreq = (float)i / (float)numPoints; // 0..1 (Logarithmic would be better)
            float mag = m_node->getMagnitudeResponse(normFreq);
            
            // Map magnitude to Y (Gain is usually -24..+24 dB, but mag is linear)
            // Simplified: 1.0 mag is center Y
            float py = m_bounds.h * 0.5f - (std::log10(mag + 0.0001f) * 20.0f * 2.0f); 
            py = std::clamp(py, 0.0f, m_bounds.h);
            points.push_back({(float)i * step, py});
        }
        batcher.drawCurve(points, 2.0f, Theme::Emerald.r, Theme::Emerald.g, Theme::Emerald.b, 1.0f);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        Component::render(batcher, dt, screenW, screenH);
    }

private:
    BiquadFilterNode* m_node;
};

} // namespace Beam

#endif