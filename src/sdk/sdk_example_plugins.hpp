/**
 * @file sdk_example_gain.hpp
 * @brief Example SDK plugin: Simple Gain
 * 
 * Demonstrates minimal SDK usage - entire plugin in ~20 lines.
 */

#ifndef SDK_EXAMPLE_GAIN_HPP
#define SDK_EXAMPLE_GAIN_HPP

#include "sdk/beam_sdk.hpp"

namespace Beam {
namespace Examples {

/**
 * @class ExampleGain
 * @brief Minimal gain plugin demonstrating SDK usage
 */
class ExampleGain : public SDK::BeamPlugin {
public:
    ExampleGain(int, float) : BeamPlugin("SDK Gain", "Examples") {
        setAuthor("Beam Audio SDK Team");
        setDescription("A simple stereo gain plugin with panning.");
        setVersion("1.1.0");

        // Declare parameters - auto-generates UI, serialization, smoothing
        m_gain = &addFloatParam("Gain", 0.0f, 2.0f, 1.0f);
        m_pan = &addFloatParam("Pan", -1.0f, 1.0f, 0.0f);
    }
    
    void process(float** io, int frames) override {
        for (int i = 0; i < frames; i++) {
            float g = m_gain->getNextValue();
            float p = m_pan->getNextValue();
            
            // Simple panning law
            float panL = std::sqrt(0.5f * (1.0f - p));
            float panR = std::sqrt(0.5f * (1.0f + p));
            
            if (io[0]) io[0][i] *= g * panL;
            if (io[1]) io[1][i] *= g * panR;
        }
    }
    
private:
    Parameter* m_gain = nullptr;
    Parameter* m_pan = nullptr;
};

/**
 * @class ExampleFilter
 * @brief Simple one-pole lowpass filter
 */
class ExampleFilter : public SDK::BeamPlugin {
public:
    ExampleFilter(int, float sr) : BeamPlugin("SDK Filter", "Examples") {
        m_sampleRate = sr;
        m_cutoff = &addLogParam("Cutoff", 20.0f, 20000.0f, 1000.0f);
        m_resonance = &addFloatParam("Resonance", 0.0f, 1.0f, 0.0f);
    }
    
    void prepareToPlay(float sampleRate, int blockSize) override {
        m_sampleRate = sampleRate;
        m_z1[0] = m_z1[1] = 0.0f;
    }
    
    void resetState() override {
        m_z1[0] = m_z1[1] = 0.0f;
    }
    
    void process(float** io, int frames) override {
        for (int i = 0; i < frames; i++) {
            float fc = m_cutoff->getNextValue();
            float coeff = 1.0f - std::exp(-2.0f * 3.14159f * fc / m_sampleRate);
            
            for (int ch = 0; ch < 2; ch++) {
                m_z1[ch] += coeff * (io[ch][i] - m_z1[ch]);
                io[ch][i] = m_z1[ch];
            }
        }
    }
    
    // Custom UI with spectrum display
    void paintCustomUI(QuadBatcher& g, float w, float h) override {
        SDK::drawKnob(g, 10, 20, 60, *m_cutoff, "CUTOFF");
        SDK::drawKnob(g, 80, 20, 60, *m_resonance, "RES");
    }
    
private:
    Parameter* m_cutoff = nullptr;
    Parameter* m_resonance = nullptr;
    float m_z1[2] = {0.0f, 0.0f};
};

} // namespace Examples
} // namespace Beam

#endif // SDK_EXAMPLE_GAIN_HPP
