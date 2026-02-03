#ifndef SPECTRUM_DISPLAY_HPP
#define SPECTRUM_DISPLAY_HPP

#include "interface/core/component.hpp"
#include "interface/core/theme.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Beam {

class SpectrumDisplay : public Component {
public:
    SpectrumDisplay() {
        setName("SpectrumDisplay");
    }

    void setFrequencyData(const std::vector<float>& magnitudes, float sampleRate) {
        m_magnitudes = magnitudes;
        m_sampleRate = sampleRate;
    }

    void paint(QuadBatcher& batcher) override {
        float w = m_bounds.w;
        float h = m_bounds.h;

        // 1. CRT Background
        batcher.drawRoundedRect(0, 0, w, h, 6.0f, 0.5f, 0.05f, 0.08f, 0.08f, 1.0f);
        
        drawGraticule(batcher, w, h);

        if (m_magnitudes.empty()) return;

        float minFreq = 20.0f;
        float maxFreq = m_sampleRate / 2.0f;
        if (maxFreq <= minFreq) maxFreq = 20000.0f;
        
        std::vector<std::pair<float, float>> points;
        points.reserve((size_t)w); 

        for (float x = 0; x < w; x += 2.0f) {
            float t = x / w;
            float freq = minFreq * std::pow(maxFreq / minFreq, t);
            
            size_t idx = (size_t)(freq / (m_sampleRate / 2.0f) * m_magnitudes.size());
            if (idx >= m_magnitudes.size()) idx = m_magnitudes.size() - 1;
            
            float mag = m_magnitudes[idx];
            float db = 20.0f * std::log10(mag + 1e-9f);
            
            float minDb = -100.0f;
            float maxDb = 0.0f;
            float normY = (db - minDb) / (maxDb - minDb);
            normY = std::clamp(normY, 0.0f, 1.0f);
            
            points.push_back({x, h - (normY * h)});
        }
        
        float r=0.2f, g=0.9f, b=0.8f;
        if (points.size() > 1) {
            batcher.drawCurve(points, 4.0f, r, g, b, 0.2f);
            batcher.drawCurve(points, 1.5f, 0.8f, 1.0f, 0.9f, 0.9f);
        }
        
        for (float y = 0; y < h; y += 3.0f) {
            batcher.drawQuad(0, y, w, 1, 0.0f, 0.0f, 0.0f, 0.15f);
        }
        
        batcher.drawRoundedGradientRect(0, 0, w, h, 6.0f, 0.5f,
                                       1.0f, 1.0f, 1.0f, 0.05f,
                                       1.0f, 1.0f, 1.0f, 0.0f);
    }
    
    void drawGraticule(QuadBatcher& batcher, float w, float h) {
        float minFreq = 20.0f;
        float maxFreq = m_sampleRate / 2.0f;
        if (maxFreq <= minFreq) maxFreq = 20000.0f;

        float freqs[] = {100.0f, 1000.0f, 10000.0f};
        for (float f : freqs) {
            if (f > maxFreq) break;
            float t = std::log(f / minFreq) / std::log(maxFreq / minFreq);
            float x = t * w;
            
            batcher.drawQuad(x, 0, 1, h, 0.3f, 0.5f, 0.4f, 0.3f);
            
            std::string label = (f >= 1000) ? std::to_string((int)f/1000) + "k" : std::to_string((int)f);
            batcher.drawText(label, x + 2, h - 12, 9, 0.3f, 0.5f, 0.4f, 0.8f);
        }
        
        float dbs[] = {-6.0f, -12.0f, -24.0f, -48.0f};
        for (float db : dbs) {
            float minDb = -100.0f;
            float maxDb = 0.0f;
            float norm = (db - minDb) / (maxDb - minDb);
            float y = h - (norm * h);
            
            batcher.drawQuad(0, y, w, 1, 0.3f, 0.5f, 0.4f, 0.3f);
            
            std::stringstream ss;
            ss << (int)db;
            batcher.drawText(ss.str(), 2, y - 8, 9, 0.3f, 0.5f, 0.4f, 0.8f);
        }
    }

private:
    std::vector<float> m_magnitudes;
    float m_sampleRate = 44100.0f;
};

} // namespace Beam

#endif // SPECTRUM_DISPLAY_HPP
