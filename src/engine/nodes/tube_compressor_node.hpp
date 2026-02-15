#ifndef TUBE_COMPRESSOR_NODE_HPP
#define TUBE_COMPRESSOR_NODE_HPP

#include "sdk/beam_sdk.hpp"
#include "engine/dsp/dsp_utils.hpp"
#include <cmath>

namespace Beam {

/**
 * @class TubeCompressorNode
 * @brief A compressor with a soft-knee and tube saturation stage.
 */
class TubeCompressorNode : public SDK::BeamPlugin {
public:
    TubeCompressorNode(int, float sr) : BeamPlugin("Tube Comp", "Dynamics"), m_sampleRate(sr) {
        threshold = &addFloatParam("Threshold", -60.0f, 0.0f, -20.0f);
        ratio = &addFloatParam("Ratio", 1.0f, 20.0f, 4.0f);
        attack = &addFloatParam("Attack", 1.0f, 100.0f, 10.0f);
        release = &addFloatParam("Release", 10.0f, 500.0f, 100.0f);
        drive = &addFloatParam("Drive", 0.0f, 12.0f, 0.0f);
    }

    void process(float** io, int frames) override {
        float threshDB = threshold->getValue();
        float rat = ratio->getValue();
        float att = attack->getValue() * 0.001f;
        float rel = release->getValue() * 0.001f;
        float drv = std::pow(10.0f, drive->getValue() / 20.0f);

        float attCoef = (att > 0) ? std::exp(-1.0f / (m_sampleRate * att)) : 0.0f;
        float relCoef = (rel > 0) ? std::exp(-1.0f / (m_sampleRate * rel)) : 0.0f;

        for (int i = 0; i < frames; ++i) {
            float detector = 0.0f;
            for(int ch=0; ch<2; ++ch) if(io[ch]) detector += std::abs(io[ch][i]);
            detector *= 0.5f;

            // Ballistics
            if (detector > m_env) m_env = attCoef * m_env + (1.0f - attCoef) * detector;
            else m_env = relCoef * m_env + (1.0f - relCoef) * detector;
            
            // Gain Reduction
            float envDB = 20.0f * std::log10(m_env + 1e-9f);
            float gainDB = 0.0f;
            if (envDB > threshDB) {
                gainDB = (threshDB - envDB) * (1.0f - 1.0f / rat);
            }
            
            float gain = std::pow(10.0f, gainDB / 20.0f);
            
            for(int ch=0; ch<2; ++ch) {
                if(!io[ch]) continue;
                float out = io[ch][i] * gain * drv;
                out = std::tanh(out); 
                io[ch][i] = flush_denormal(out);
            }
        }
    }

    void prepareToPlay(float sr, int) override {
        m_sampleRate = sr;
    }

private:
    float m_sampleRate;
    float m_env = 0.0f;
    Parameter *threshold, *ratio, *attack, *release, *drive;
};

REGISTER_BEAM_PLUGIN(TubeCompressorNode)

} // namespace Beam

#endif
 // TUBE_COMPRESSOR_NODE_HPP






