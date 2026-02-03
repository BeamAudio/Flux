#ifndef BUS_NODE_HPP
#define BUS_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/dsp/simd_utils.hpp"
#include <vector>

namespace Beam {

/**
 * @class BusProcessor
 * @brief Sums all connected inputs and passes them to the output.
 */
class BusProcessor : public FluxProcessor {
public:
    void process(const float** inputs, float** outputs, int frames) override {
        float* out = outputs[0];
        int total = frames * 2;
        
        // Input summing is handled by the Graph's routing (SignalRoute).
        // Since we have multiple input ports, they are all summed into their 
        // respective buffers before process() is called.
        // But a BUS usually wants to sum all its DIFFERENT input ports into one output.
        
        std::fill(out, out + total, 0.0f);
        
        // Sum all stereo input pairs (8 inputs provided in BusNode)
        for (int i = 0; i < 8; ++i) {
            if (inputs[i]) {
                SIMD::add(out, inputs[i], total);
            }
        }
    }
};

/**
 * @class BusNode
 * @brief A summing point for multiple tracks. 
 * Facilitates sub-mixing and grouping.
 */
class BusNode : public FluxNode {
public:
    BusNode() {
        addParameter(std::make_shared<Parameter>("Volume", 0.0f, 2.0f, 1.0f));
    }

    std::string getName() const override { return "Bus"; }

    std::vector<Port> getInputPorts() const override {
        return {
            {"In 1", 2}, {"In 2", 2}, {"In 3", 2}, {"In 4", 2},
            {"In 5", 2}, {"In 6", 2}, {"In 7", 2}, {"In 8", 2}
        };
    }

    std::vector<Port> getOutputPorts() const override {
        return { {"Sum Out", 2} };
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<BusProcessor>();
    }
};

} // namespace Beam

#endif // BUS_NODE_HPP
