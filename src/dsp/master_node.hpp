#ifndef MASTER_NODE_HPP
#define MASTER_NODE_HPP

#include "flux_node.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>

namespace Beam {

class MasterNode : public FluxNode {
public:
    MasterNode(int bufferSize) {
        setupBuffers(1, 0, bufferSize, 2); // 1 Stereo Input, 0 Outputs
        m_currentPeak.store(0.0f);
    }

    void process(int frames) override {
        // Master node doesn't need to do much besides being a sink in the graph
        // The AudioEngine will pull data from its input buffer
        
        float* in = getInputBuffer(0);
        float peak = 0.0f;
        for (int i = 0; i < frames * 2; ++i) {
            float s = std::abs(in[i]);
            if (s > peak) peak = s;
        }
        
        // Simple decay for visual persistence
        float prev = m_currentPeak.load();
        if (peak < prev) peak = prev * 0.95f; 
        
        m_currentPeak.store(peak);
    }

    float getPeakLevel() const { return m_currentPeak.load(); }

    std::string getName() const override { return "Master"; }
    
    std::vector<Port> getInputPorts() const override {
        return { {"Stereo In", 2} };
    }
    
    std::vector<Port> getOutputPorts() const override {
        return {};
    }

private:
    std::atomic<float> m_currentPeak;
};

} // namespace Beam

#endif // MASTER_NODE_HPP
