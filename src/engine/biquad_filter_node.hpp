#ifndef BIQUAD_FILTER_NODE_HPP
#define BIQUAD_FILTER_NODE_HPP

#include "audio_node.hpp"
#include "dsp_utils.hpp"
#include <cmath>

namespace Beam {

enum class FilterType {
    LowPass,
    HighPass
};

class BiquadFilterNode : public AudioNode {
public:
    BiquadFilterNode(FilterType type, float frequency, float q, float sampleRate) 
        : m_type(type), m_frequency(frequency), m_q(q), m_sampleRate(sampleRate) {
        calculateCoefficients();
    }

    void process(float* buffer, int frames, int channels, size_t startFrame = 0) override {
        if (m_x1.size() != (size_t)channels) {
            m_x1.assign(channels, 0.0f);
            m_x2.assign(channels, 0.0f);
            m_y1.assign(channels, 0.0f);
            m_y2.assign(channels, 0.0f);
        }

        for (int i = 0; i < frames; ++i) {
            for (int c = 0; c < channels; ++c) {
                float x = buffer[i * channels + c];
                float y = (m_b0 / m_a0) * x + (m_b1 / m_a0) * m_x1[c] + (m_b2 / m_a0) * m_x2[c]
                          - (m_a1 / m_a0) * m_y1[c] - (m_a2 / m_a0) * m_y2[c];
                
                y = flush_denormal(y);

                m_x2[c] = m_x1[c];
                m_x1[c] = x;
                m_y2[c] = m_y1[c];
                m_y1[c] = y;
                
                buffer[i * channels + c] = y;
            }
        }
    }

    std::string getName() const override { return "Biquad Filter"; }

    void setCutoff(float freq) {
        m_frequency = freq;
        calculateCoefficients();
    }

    void setQ(float q) {
        m_q = q;
        calculateCoefficients();
    }

private:
    void calculateCoefficients() {
        float omega = 2.0f * 3.1415926535f * m_frequency / m_sampleRate;
        float cosOmega = std::cos(omega);
        float alpha = std::sin(omega) / (2.0f * m_q);

        if (m_type == FilterType::LowPass) {
            m_b0 = (1.0f - cosOmega) / 2.0f;
            m_b1 = 1.0f - cosOmega;
            m_b2 = (1.0f - cosOmega) / 2.0f;
            m_a0 = 1.0f + alpha;
            m_a1 = -2.0f * cosOmega;
            m_a2 = 1.0f - alpha;
        } else { // HighPass
            m_b0 = (1.0f + cosOmega) / 2.0f;
            m_b1 = -(1.0f + cosOmega);
            m_b2 = (1.0f + cosOmega) / 2.0f;
            m_a0 = 1.0f + alpha;
            m_a1 = -2.0f * cosOmega;
            m_a2 = 1.0f - alpha;
        }
    }

    FilterType m_type;
    float m_frequency, m_q, m_sampleRate;
    float m_b0, m_b1, m_b2, m_a0, m_a1, m_a2;
    std::vector<float> m_x1, m_x2, m_y1, m_y2;
};

} // namespace Beam

#endif // BIQUAD_FILTER_NODE_HPP
