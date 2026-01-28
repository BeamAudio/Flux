#ifndef TUBE_COMPRESSOR_NODE_HPP
#define TUBE_COMPRESSOR_NODE_HPP

#include "flux_plugin.hpp"
#include "dsp_utils.hpp"
#include <cmath>

namespace Beam {

/**
 * @class TubeCompressorNode
 * @brief A compressor with a soft-knee and tube saturation stage.
 */
class TubeCompressorNode : public FluxPlugin {
public:
    TubeCompressorNode(int bufferSize, float sampleRate) 
        : FluxPlugin("Tube Comp", bufferSize, sampleRate) 
    {
        addParam("Threshold", -60.0f, 0.0f, -20.0f);
        addParam("Ratio", 1.0f, 20.0f, 4.0f);
        addParam("Attack", 1.0f, 100.0f, 10.0f);
        addParam("Release", 10.0f, 500.0f, 100.0f);
        addParam("Drive", 0.0f, 12.0f, 0.0f); // Tube Saturation
        
        m_envelope.store(0.0f);
    }

    void processBlock(const float* input, float* output, int totalSamples) override {
        float threshDB = getParam("Threshold");
        float ratio = getParam("Ratio");
        float attack = getParam("Attack") * 0.001f;
        float release = getParam("Release") * 0.001f;
        float drive = std::pow(10.0f, getParam("Drive") / 20.0f);

        float attCoef = std::exp(-1.0f / (getSampleRate() * attack));
        float relCoef = std::exp(-1.0f / (getSampleRate() * release));

        for (int i = 0; i < totalSamples; ++i) {
            float in = input[i];
            float absIn = std::abs(in);

            // Ballistics
            float env = m_envelope.load();
            if (absIn > env) env = attCoef * env + (1.0f - attCoef) * absIn;
            else env = relCoef * env + (1.0f - relCoef) * absIn;
            m_envelope.store(env);

            // Gain Reduction
            float envDB = 20.0f * std::log10(env + 1e-9f);
            float gainDB = 0.0f;
            if (envDB > threshDB) {
                gainDB = (threshDB - envDB) * (1.0f - 1.0f / ratio);
            }
            
            float gain = std::pow(10.0f, gainDB / 20.0f);
            float out = in * gain;

            // Tube Saturation (Soft Clip)
            out *= drive;
            out = std::tanh(out); 
            
            output[i] = flush_denormal(out);
        }
    }

private:
    std::atomic<float> m_envelope;
};

} // namespace Beam

#endif // TUBE_COMPRESSOR_NODE_HPP

