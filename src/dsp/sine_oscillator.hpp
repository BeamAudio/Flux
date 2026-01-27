#ifndef SINE_OSCILLATOR_HPP
#define SINE_OSCILLATOR_HPP

#include "audio_node.hpp"
#include <cmath>

namespace Beam {

class SineOscillator : public AudioNode {
public:
    SineOscillator(float frequency, float sampleRate) 
        : m_frequency(frequency), m_sampleRate(sampleRate), m_phase(0.0f) {}

    void process(float* buffer, int frames, int channels) override {
        float phaseIncrement = (2.0f * 3.1415926535f * m_frequency) / m_sampleRate;
        for (int i = 0; i < frames; ++i) {
            float sample = std::sin(m_phase) * 0.2f; // Low volume for safety
            for (int c = 0; c < channels; ++c) {
                buffer[i * channels + c] += sample;
            }
            m_phase += phaseIncrement;
            if (m_phase >= 2.0f * 3.1415926535f) m_phase -= 2.0f * 3.1415926535f;
        }
    }

    std::string getName() const override { return "Sine Oscillator"; }

private:
    float m_frequency;
    float m_sampleRate;
    float m_phase;
};

} // namespace Beam

#endif // SINE_OSCILLATOR_HPP
