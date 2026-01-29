#ifndef SIMPLE_GAIN_PROCESSOR_HPP
#define SIMPLE_GAIN_PROCESSOR_HPP

#include "flux_node.hpp"
#include "../engine/audio_processor_value_tree_state.hpp"
#include <memory>

namespace Beam {

/**
 * @class SimpleGainProcessor
 * @brief A simple gain processor demonstrating the new API
 */
class SimpleGainProcessor : public FluxNode {
public:
    SimpleGainProcessor();
    ~SimpleGainProcessor() override;

    void process(int frames) override;

    std::string getName() const override { return "Simple Gain"; }
    std::vector<Port> getInputPorts() const override;
    std::vector<Port> getOutputPorts() const override;

    // Getters for the parameters
    float getGain() const;

private:
    AudioProcessorValueTreeState m_valueTreeState;
    std::shared_ptr<Parameter> m_gainParameter;
};

} // namespace Beam

#endif // SIMPLE_GAIN_PROCESSOR_HPP


