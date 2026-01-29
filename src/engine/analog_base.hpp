#ifndef ANALOG_BASE_HPP
#define ANALOG_BASE_HPP

#include <cmath>
#include <random>
#include <algorithm>
#include <vector>

namespace Beam {

/**
 * @class AnalogBase
 * @brief Provides reusable physics-based algorithms for analog emulation.
 */
class AnalogBase {
public:
    /**
     * @brief Langevin function approximation for magnetic hysteresis simulation.
     * High-level saturation that mimics how iron-oxide particles on tape respond to flux.
     */
    static inline float saturateLangevin(float x, float drive) {
        float input = x * drive;
        // Approximation of Langevin L(x) = coth(x) - 1/x
        // We use a scaled tanh for better real-time performance and musicality
        return std::tanh(input); 
    }

    /**
     * @brief Simulates odd-harmonic distortion typical of transformer cores.
     */
    static inline float saturateTransformer(float x, float iron) {
        // Cubic non-linearity for odd harmonics
        float x3 = x * x * x;
        return x - (iron * 0.1f * x3);
    }

    /**
     * @class OnePoleFilter
     * @brief Simple 6dB/oct filter for simulating cable capacitance or basic tone shaping.
     */
    class OnePoleFilter {
    public:
        void setCutoff(float freq, float sampleRate) {
            m_b1 = std::exp(-2.0f * 3.14159265f * freq / sampleRate);
            m_a0 = 1.0f - m_b1;
        }

        inline float process(float x) {
            m_z1 = x * m_a0 + m_z1 * m_b1;
            return m_z1;
        }

        void reset() { m_z1 = 0.0f; }

    private:
        float m_a0 = 1.0f, m_b1 = 0.0f, m_z1 = 0.0f;
    };

    /**
     * @class WowFlutterGenerator
     * @brief Generates physical tape speed fluctuations.
     */
    class WowFlutterGenerator {
    public:
        WowFlutterGenerator(float sampleRate) : m_sampleRate(sampleRate) {
            m_rng.seed(std::random_device()());
        }

        void setIntensity(float wow, float flutter) {
            m_wowDepth = wow;
            m_flutterDepth = flutter;
        }

        float next() {
            // Wow (Slow LFO 0.5Hz)
            m_wowPhase += 0.5f / m_sampleRate;
            if (m_wowPhase > 1.0f) m_wowPhase -= 1.0f;
            float w = std::sin(m_wowPhase * 6.283185f) * m_wowDepth;

            // Flutter (Faster stochastic noise)
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            float noise = dist(m_rng);
            m_flutterLP.setCutoff(25.0f, m_sampleRate); // Flutter is focused in 10-50Hz range
            float f = m_flutterLP.process(noise) * m_flutterDepth;

            return w + f;
        }

    private:
        float m_sampleRate;
        float m_wowDepth = 0.0f;
        float m_flutterDepth = 0.0f;
        float m_wowPhase = 0.0f;
        OnePoleFilter m_flutterLP;
        std::mt19937 m_rng;
    };
};

} // namespace Beam

#endif // ANALOG_BASE_HPP


