#ifndef SPECTRUM_EDITOR_HPP
#define SPECTRUM_EDITOR_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include "engine/plugins/flux_plugin.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include "interface/widgets/spectrum_display.hpp"

namespace Beam {

class SpectrumEditor : public Component {
public:
    SpectrumEditor(FluxPlugin* node) : m_node(node) {
        setName("SpectrumEditor");
        m_display = std::make_shared<SpectrumDisplay>();
        addChildComponent(m_display);
    }

    void getPreferredSize(float& w, float& h) const override {
        w = 480; 
        h = 240;
    }
    
    void setBounds(float x, float y, float width, float height) override {
        Component::setBounds(x, y, width, height);
        if (m_display) m_display->setBounds(0, 0, width, height);
    }

    void render(QuadBatcher& batcher, float dt, float screenW, float screenH) override {
        if (m_node) {
            m_node->updateVisuals(); 
            if (m_display) updateSpectrumData();
        }
        Component::render(batcher, dt, screenW, screenH);
    }

private:
    FluxPlugin* m_node;
    std::shared_ptr<SpectrumDisplay> m_display;
    std::vector<float> m_spectrumData;
    
    void updateSpectrumData() {
        // Synthesize high-res spectrum from EQ bands for visualization
        std::vector<std::string> freqs = {"31Hz", "63Hz", "125Hz", "250Hz", "500Hz", "1000Hz", "2000Hz", "4000Hz", "8000Hz", "16000Hz"};
        std::vector<float> bandGains;
        
        // Read params
        for (const auto& f : freqs) {
            auto p = m_node->getParameter(f);
            float val = p ? p->getValue() : 0.0f; 
            // Convert dB to linear magnitude (approx)
            // Display expects linear magnitude usually, but here our display converts to dB internally? 
            // SpectrumDisplay takes "magnitudes" and does 20*log10.
            // If params are in dB (e.g. +3dB), we need to convert to linear first.
            bandGains.push_back(std::pow(10.0f, val / 20.0f));
        }
        
        // Interpolate to 512 points
        size_t numPoints = 512;
        if (m_spectrumData.size() != numPoints) m_spectrumData.resize(numPoints);
        
        // Simple interpolation logic
        // Map 0..512 to log-freq, find nearest bands, interpolate
        // Since bands are octave spaced (logarithmic), they are linearly spaced in log-domain.
        // We can just interpolate the vector indices if we assume linear mapping of indices = log mapping of freq
        
        for (size_t i = 0; i < numPoints; ++i) {
            float t = (float)i / (float)(numPoints - 1); // 0..1
            
            float bandPos = t * (bandGains.size() - 1);
            size_t idx = (size_t)bandPos;
            float frac = bandPos - idx;
            
            float v1 = bandGains[idx];
            float v2 = (idx + 1 < bandGains.size()) ? bandGains[idx+1] : v1;
            
            // Cubic smoothing or Linear? Linear is pointier. Cosine is nicer.
            float mu2 = (1.0f - std::cos(frac * 3.14159f)) / 2.0f;
            float val = v1 * (1.0f - mu2) + v2 * mu2;
            
            m_spectrumData[i] = val;
        }
        
        m_display->setFrequencyData(m_spectrumData, 44100.0f);
    }
};

} // namespace Beam
#endif
