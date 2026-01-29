#ifndef FILTER_GRAPH_HPP
#define FILTER_GRAPH_HPP

#include "component.hpp"
#include "../engine/biquad_filter_node.hpp"
#include <vector>
#include <cmath>

namespace Beam {

class FilterGraph : public Component {
public:
    FilterGraph(BiquadFilterNode* node) : m_node(node) {
        setBounds(0, 0, 130, 80);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (!m_node) return;

        // Background
        batcher.drawRoundedRect(m_bounds.x, m_bounds.y, m_bounds.w, m_bounds.h, 4.0f, 1.0f, 0.05f, 0.05f, 0.06f, 1.0f);
        
        // Grid lines (approx log scale)
        for (float f = 0.1f; f < 1.0f; f += 0.2f) {
            float lx = m_bounds.x + f * m_bounds.w;
            batcher.drawQuad(lx, m_bounds.y, 1, m_bounds.h, 0.15f, 0.15f, 0.15f, 1.0f);
        }

        std::vector<std::pair<float, float>> points;
        const int numPoints = 40;
        
        for (int i = 0; i < numPoints; ++i) {
            float t = (float)i / (numPoints - 1);
            // Logarithmic frequency mapping for display
            float freq = 20.0f * std::pow(1000.0f, t);
            float normFreq = freq / (44100.0f * 0.5f);
            
            float mag = m_node->getMagnitudeResponse(normFreq);
            float db = 20.0f * std::log10(mag + 1e-6f);
            
            // Map -24dB..+24dB to UI height
            float py = m_bounds.y + m_bounds.h * 0.5f - (db * (m_bounds.h * 0.02f));
            py = std::clamp(py, m_bounds.y + 2, m_bounds.y + m_bounds.h - 2);
            
            points.push_back({m_bounds.x + t * m_bounds.w, py});
        }

        batcher.drawCurve(points, 2.0f, 0.2f, 0.6f, 1.0f, 1.0f);
    }

private:
    BiquadFilterNode* m_node;
};

} // namespace Beam

#endif // FILTER_GRAPH_HPP



