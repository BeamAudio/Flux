#ifndef MASTER_NODE_HPP
#define MASTER_NODE_HPP

#include "flux_node.hpp"
#include "analog_base.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>

namespace Beam {

/**
 * @class MasterNode
 * @brief The final sink in the audio graph. Handles global volume, metering, and transformer saturation.
 */
class MasterNode : public FluxNode {
public:
    MasterNode(int bufferSize) {
        setupBuffers(1, 0, bufferSize, 2); // 1 Stereo Input, 0 Outputs
        m_currentPeak.store(0.0f);
        addParameter(std::make_shared<Parameter>("Master Gain", 0.0f, 1.5f, 1.0f));
        addParameter(std::make_shared<Parameter>("Transformer", 0.0f, 1.0f, 0.2f));
        addParameter(std::make_shared<Parameter>("Crosstalk", 0.0f, 0.1f, 0.01f));
    }

    void process(int frames) override {
        float* in = getInputBuffer(0);
        float gain = getParameter("Master Gain")->getValue();
        float iron = getParameter("Transformer")->getValue();
        float xtalk = getParameter("Crosstalk")->getValue();
        float peak = 0.0f;

        for (int i = 0; i < frames; ++i) {
            float& L = in[i * 2];
            float& R = in[i * 2 + 1];

            // 1. Crosstalk (Analog leakage)
            float lLeak = R * xtalk;
            float rLeak = L * xtalk;
            L = L * (1.0f - xtalk) + lLeak;
            R = R * (1.0f - xtalk) + rLeak;

            // 2. Transformer Saturation & Gain
            L = AnalogBase::saturateTransformer(L * gain, iron);
            R = AnalogBase::saturateTransformer(R * gain, iron);

            float s = (std::max)(std::abs(L), std::abs(R));
            if (s > peak) peak = s;
        }

        float prev = m_currentPeak.load();
        if (peak < prev) peak = prev * 0.95f; // Visual decay

        m_currentPeak.store(peak);
    }

    float getPeakLevel() const { return m_currentPeak.load(); }

    std::string getName() const override { return "Master"; }

    std::vector<FluxNode::Port> getInputPorts() const override {
        return { {"Stereo In", 2} };
    }

    std::vector<FluxNode::Port> getOutputPorts() const override {
        return {};
    }


private:
    std::atomic<float> m_currentPeak;
};

} // namespace Beam

#endif // MASTER_NODE_HPP





