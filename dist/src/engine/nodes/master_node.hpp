#ifndef MASTER_NODE_HPP
#define MASTER_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/nodes/analog_base.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>

namespace Beam {

/**
 * @class MasterNode
 * @brief The final sink in the audio graph. Handles global volume, metering, and transformer saturation.
 */
class MasterProcessor : public FluxProcessor {
public:
    MasterProcessor(std::shared_ptr<MeterSource> meterSource) : m_meterSource(meterSource) {}

    void updateParameters(const float* paramValues) override {
        // Map order: Master Gain, Transformer, Crosstalk
        m_gain = paramValues[0];
        m_iron = paramValues[1];
        m_xtalk = paramValues[2];
    }

    void process(const float** inputs, float** outputs, int frames) override {
        const float* in = inputs[0];
        float* out = outputs[0];
        
        float peakL = 0.0f;
        float peakR = 0.0f;

        if (!in) {
            std::fill(out, out + frames * 2, 0.0f);
            if (m_meterSource) {
                m_meterSource->updateMeter(0, 0);
                m_meterSource->updateMeter(1, 0);
            }
            return;
        }

        for (int i = 0; i < frames; ++i) {
            float L = in[i * 2];
            float R = in[i * 2 + 1];

            // 1. Crosstalk
            float lLeak = R * m_xtalk;
            float rLeak = L * m_xtalk;
            L = L * (1.0f - m_xtalk) + lLeak;
            R = R * (1.0f - m_xtalk) + rLeak;

            // 2. Transformer Saturation & Gain
            L = AnalogBase::saturateTransformer(L * m_gain, m_iron);
            R = AnalogBase::saturateTransformer(R * m_gain, m_iron);

            if (std::abs(L) > peakL) peakL = std::abs(L);
            if (std::abs(R) > peakR) peakR = std::abs(R);
            
            // Write to output
            out[i * 2] = L;
            out[i * 2 + 1] = R;
        }

        if (m_meterSource) {
            m_meterSource->updateMeter(0, peakL);
            m_meterSource->updateMeter(1, peakR);
        }
    }

private:
    float m_gain = 1.0f;
    float m_iron = 0.2f;
    float m_xtalk = 0.01f;
    std::shared_ptr<MeterSource> m_meterSource;
};

class MasterNode : public FluxNode {
public:
    MasterNode(int bufferSize) {
        m_meterSource->addMeter("Output L");
        m_meterSource->addMeter("Output R");

        addParameter(std::make_shared<Parameter>("Master Gain", 0.0f, 1.5f, 1.0f));
        addParameter(std::make_shared<Parameter>("Transformer", 0.0f, 1.0f, 0.2f));
        addParameter(std::make_shared<Parameter>("Crosstalk", 0.0f, 0.1f, 0.01f));
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<MasterProcessor>(m_meterSource);
    }

    float getPeakLevel() const { 
        return (std::max)(m_meterSource->getValue(0), m_meterSource->getValue(1)); 
    }

    std::string getName() const override { return "Master"; }
    std::vector<FluxNode::Port> getInputPorts() const override { return { {"Stereo In", 2} }; }
    std::vector<FluxNode::Port> getOutputPorts() const override { return { {"Stereo Out", 2} }; }

    std::shared_ptr<Component> createEditor(const NodeEditorContext& context) override;

private:
};

} // namespace Beam

#endif // MASTER_NODE_HPP





