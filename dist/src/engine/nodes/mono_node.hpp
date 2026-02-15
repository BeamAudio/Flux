#ifndef MONO_GAIN_NODE_HPP
#define MONO_GAIN_NODE_HPP

#include "engine/core/flux_node.hpp"
#include "engine/plugins/plugin_registry.hpp"
#include <algorithm>

namespace Beam {
// ... (processor remains same)
/**
 * @class MonoGainNode
 * @brief A standard 1-in / 1-out utility node for signal chains.
 */
class MonoGainNode : public FluxNode {
public:
    MonoGainNode(int bufferSize, float sampleRate) {
        addParameter(std::make_shared<Parameter>("Gain", 0.0f, 2.0f, 1.0f));
    }

    std::shared_ptr<FluxProcessor> createProcessor() override {
        return std::make_shared<MonoGainProcessor>();
    }

    std::string getName() const override { return "Mono Gain"; }
    
    // Explicit 1x1 I/O
    std::vector<Port> getInputPorts() const override { return { {"Mono In", 1} }; }
    std::vector<Port> getOutputPorts() const override { return { {"Mono Out", 1} }; }
};

REGISTER_PLUGIN(MonoGainNode, "Mono Gain");

} // namespace Beam

#endif
