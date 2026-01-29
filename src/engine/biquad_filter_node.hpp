#ifndef BIQUAD_FILTER_NODE_HPP
#define BIQUAD_FILTER_NODE_HPP

#include "audio_node.hpp"
#include "dsp_utils.hpp"
#include <cmath>

namespace Beam {

enum class FilterType {
    LowPass,
    HighPass,
    Peaking,
    LowShelf,
    HighShelf
};

class BiquadFilterNode : public AudioNode {
public:
    BiquadFilterNode(FilterType type, float frequency, float q, float sampleRate) 
        : m_type(type), m_frequency(frequency), m_q(q), m_sampleRate(sampleRate), m_gain(0.0f) {
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

    // Single sample processing for mono/legacy use (assumes channel 0)
    // WARN: Use with caution on interleaved data
    float process(float input) {
        if (m_x1.empty()) { m_x1.resize(1,0); m_x2.resize(1,0); m_y1.resize(1,0); m_y2.resize(1,0); }
        float x = input;
        float y = (m_b0 / m_a0) * x + (m_b1 / m_a0) * m_x1[0] + (m_b2 / m_a0) * m_x2[0]
                  - (m_a1 / m_a0) * m_y1[0] - (m_a2 / m_a0) * m_y2[0];
        y = flush_denormal(y);
        m_x2[0] = m_x1[0]; m_x1[0] = x;
        m_y2[0] = m_y1[0]; m_y1[0] = y;
        return y;
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

    void setGain(float db) {
        m_gain = db;
        calculateCoefficients();
    }

    /**
     * @brief Calculates the magnitude response at a given normalized frequency (0..1, where 1 is Nyquist).
     */
    float getMagnitudeResponse(float normalizedFreq) {
        float w = normalizedFreq * 3.1415926535f;
        float cosW = std::cos(w);
        float cos2W = std::cos(2.0f * w);

        float num = m_b0 * m_b0 + m_b1 * m_b1 + m_b2 * m_b2 + 2.0f * (m_b0 * m_b1 + m_b1 * m_b2) * cosW + 2.0f * m_b0 * m_b2 * cos2W;
        float den = m_a0 * m_a0 + m_a1 * m_a1 + m_a2 * m_a2 + 2.0f * (m_a0 * m_a1 + m_a1 * m_a2) * cosW + 2.0f * m_a0 * m_a2 * cos2W;

        return std::sqrt((std::max)(0.0f, num / den));
    }

private:
    void calculateCoefficients() {
        float omega = 2.0f * 3.1415926535f * m_frequency / m_sampleRate;
        float cosOmega = std::cos(omega);
        float alpha = std::sin(omega) / (2.0f * m_q);
        float A = std::pow(10.0f, m_gain / 40.0f); // For shelves/peaking

        switch (m_type) {
            case FilterType::LowPass:
                m_b0 = (1.0f - cosOmega) / 2.0f;
                m_b1 = 1.0f - cosOmega;
                m_b2 = (1.0f - cosOmega) / 2.0f;
                m_a0 = 1.0f + alpha;
                m_a1 = -2.0f * cosOmega;
                m_a2 = 1.0f - alpha;
                break;
            case FilterType::HighPass:
                m_b0 = (1.0f + cosOmega) / 2.0f;
                m_b1 = -(1.0f + cosOmega);
                m_b2 = (1.0f + cosOmega) / 2.0f;
                m_a0 = 1.0f + alpha;
                m_a1 = -2.0f * cosOmega;
                m_a2 = 1.0f - alpha;
                break;
            case FilterType::Peaking:
                m_b0 = 1.0f + alpha * A;
                m_b1 = -2.0f * cosOmega;
                m_b2 = 1.0f - alpha * A;
                m_a0 = 1.0f + alpha / A;
                m_a1 = -2.0f * cosOmega;
                m_a2 = 1.0f - alpha / A;
                break;
            case FilterType::LowShelf: {
                float sqrtA = std::sqrt(A);
                m_b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha);
                m_b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
                m_b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha);
                m_a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha;
                m_a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
                m_a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha;
                break; 
            }
            case FilterType::HighShelf: {
                float sqrtA = std::sqrt(A);
                m_b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha);
                m_b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
                m_b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha);
                m_a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + 2.0f * sqrtA * alpha;
                m_a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
                m_a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - 2.0f * sqrtA * alpha;
                break;
            }
        }
    }

    FilterType m_type;
    float m_frequency, m_q, m_sampleRate, m_gain;
    float m_b0, m_b1, m_b2, m_a0, m_a1, m_a2;
    std::vector<float> m_x1, m_x2, m_y1, m_y2;
};

} // namespace Beam

#endif // BIQUAD_FILTER_NODE_HPP





