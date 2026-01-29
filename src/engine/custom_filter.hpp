#ifndef CUSTOM_FILTER_HPP
#define CUSTOM_FILTER_HPP

#include "flux_plugin.hpp"
#include <cmath>

namespace Beam {

/**
 * Example of a User-Designed Filter using the Flux SDK.
 * This is a simple One-Pole Low Pass filter.
 */
class CustomFilter : public FluxPlugin {
public:
    CustomFilter(int bufferSize, float sampleRate) 
        : FluxPlugin("User Filter", bufferSize, sampleRate) 
    {
        // 1. Define Parameters (Automatic GUI)
        addParam("Cutoff", 0.0f, 1.0f, 0.5f);
        addParam("Reso", 0.0f, 0.95f, 0.0f);
    }

    void processBlock(const float* input, float* output, int totalSamples) override {
        // 2. Read Parameters
        float cutoff = getParam("Cutoff");
        float resonance = getParam("Reso");

        // 3. DSP Kernel
        for (int i = 0; i < totalSamples; ++i) {
            // Simple low-pass smoothing
            m_z1 = m_z1 + cutoff * (input[i] - m_z1);
            
            // Add some resonance feedback (simplified)
            float val = m_z1 + (m_z1 - m_lastOut) * resonance;
            
            output[i] = val;
            m_lastOut = val;
        }
    }

private:
    float m_z1 = 0.0f;
    float m_lastOut = 0.0f;
};

} // namespace Beam

#endif // CUSTOM_FILTER_HPP






